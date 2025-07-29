#include <w32use.hpp>
#include "GetUIAccessToken.h"
#include <UserEnv.h>
#include <fstream>
#include "uiaccess.ipc.internal.hpp"
using namespace std;

static std::wstring cfgTmpKey;

DWORD SuspendProcess(HANDLE hProcess);
DWORD ResumeProcess(HANDLE hProcess);

#pragma region 服务的样板代码
static SERVICE_STATUS serviceStatus;
static SERVICE_STATUS_HANDLE serviceStatusHandle;
static std::wstring serviceName;
static void appStart();
static void ReportServiceStatus(DWORD currentState, DWORD exitCode = 0, DWORD waitHint = 0) {
	static DWORD checkPoint = 1;

	serviceStatus.dwCurrentState = currentState;
	serviceStatus.dwWin32ExitCode = exitCode;
	serviceStatus.dwWaitHint = waitHint;

	if (currentState == SERVICE_START_PENDING) {
		serviceStatus.dwControlsAccepted = 0;
	}
	else {
		serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	}

	if ((currentState == SERVICE_RUNNING) || (currentState == SERVICE_STOPPED)) {
		serviceStatus.dwCheckPoint = 0;
	}
	else {
		serviceStatus.dwCheckPoint = checkPoint++;
	}

	SetServiceStatus(serviceStatusHandle, &serviceStatus);
}
static void ServiceMain(DWORD argc, LPWSTR* argv) {
	serviceStatusHandle = RegisterServiceCtrlHandler(serviceName.c_str(), [](DWORD control) {
		if (control == SERVICE_CONTROL_STOP) {
			ReportServiceStatus(SERVICE_STOPPED);
			exit(0);
		}
	});

	if (!serviceStatusHandle) {
		return;
	}

	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwServiceSpecificExitCode = 0;

	ReportServiceStatus(SERVICE_START_PENDING);
	ReportServiceStatus(SERVICE_RUNNING);

	try {
		appStart();
	}
	catch (std::exception&) {
		ReportServiceStatus(SERVICE_STOPPED);
	}
}
#pragma endregion

/// <summary>
/// 启用 Token 的所有特权
/// </summary>
/// <param name="hToken">目标 Token。可以为空（如果为空则自动使用当前进程的）。</param>
/// <returns>是否成功。</returns>
BOOL EnableAllPrivileges(HANDLE hToken) {
	BOOL bResult = FALSE;
	HANDLE hTokenLocal = nullptr;
	DWORD dwTokenInfoSize = 0;
	PTOKEN_PRIVILEGES pTokenPrivileges = nullptr;

	// 处理令牌句柄
	if (hToken == nullptr) {
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hTokenLocal)) {
			return FALSE;
		}
		hToken = hTokenLocal;
	}

	// 获取所需缓冲区大小
	if (!GetTokenInformation(hToken, TokenPrivileges, nullptr, 0, &dwTokenInfoSize) &&
		GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		goto cleanup;
	}

	// 分配权限信息缓冲区
	pTokenPrivileges = reinterpret_cast<PTOKEN_PRIVILEGES>(malloc(dwTokenInfoSize));
	if (!pTokenPrivileges) {
		goto cleanup;
	}

	// 获取实际权限信息
	if (!GetTokenInformation(hToken, TokenPrivileges, pTokenPrivileges, dwTokenInfoSize, &dwTokenInfoSize)) {
		goto cleanup;
	}

	// 启用所有权限
	bResult = TRUE;
	for (DWORD i = 0; i < pTokenPrivileges->PrivilegeCount; ++i) {
		LUID_AND_ATTRIBUTES& la = pTokenPrivileges->Privileges[i];
		TOKEN_PRIVILEGES tp = { 1, { { la.Luid, SE_PRIVILEGE_ENABLED } } };

		if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
			bResult = FALSE;
		}
		else if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
			bResult = FALSE;
		}
	}

cleanup:
	if (pTokenPrivileges) free(pTokenPrivileges);
	if (hTokenLocal) CloseHandle(hTokenLocal);
	return bResult;
}

static int argc; static LPWSTR* argv;
static void appStart() { // 此函数由调用者处理可能的异常
	// 这里才是真正的创建进程
	// 首先从注册表获取内容
	DWORD callerPID, flag, session;
	wstring appName, appCmdLine;
	{
		RegistryKey data(HKEY_LOCAL_MACHINE, L"SOFTWARE\\" + cfgTmpKey);
		callerPID = data.get_value<DWORD>(L"callerPID");
		if (data.exists_value(L"targetAppName")) appName = data.get(L"targetAppName").get<wstring>();
		appCmdLine = data.get_value<wstring>(L"targetCmdLine");

		flag = data.get_value<DWORD>(L"flag");
		session = data.get_value<DWORD>(L"session");
	}
	// 删除注册表
	RegistryKey(HKEY_LOCAL_MACHINE, L"SOFTWARE").delete_key(cfgTmpKey);

	w32FileHandle hFileMapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, (L"Global\\" + cfgTmpKey).c_str());
	auto pIPC = (StartUIAccessProcess_IPC*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (!pIPC) {
		throw exception("Failed to open file mapping");
	}
	w32oop::util::RAIIHelper _([&pIPC] {
		UnmapViewOfFile(pIPC);
	});

	// 获取 Token
	w32ProcessHandle hCaller = OpenProcess(PROCESS_ALL_ACCESS, FALSE, callerPID);
	hCaller.validate();
	HANDLE hToken = GetUIAccessToken(session);
	if (!hToken) {
		// 获取 Token 失败，向调用者报告失败
		pIPC->status = 0x10; // 失败
		pIPC->error = GetLastError();
		throw exception("Failed");
	}

	// 创建进程
	// 设置启动信息
	flag |= CREATE_NEW_CONSOLE;// | EXTENDED_STARTUPINFO_PRESENT;
	WCHAR lpDesktop[] = L"winsta0\\default";
	STARTUPINFOEXW siex{};
	siex.StartupInfo.cb = sizeof(STARTUPINFOW);
	siex.StartupInfo.lpDesktop = lpDesktop;
	// 相关信息
	PROCESS_INFORMATION pi{};
	// 创建环境块
	LPVOID pEnv = NULL;
	if (CreateEnvironmentBlock(&pEnv, hToken, TRUE)) {
		flag |= CREATE_UNICODE_ENVIRONMENT;
	}

	auto cl = make_unique<wchar_t[]>(appCmdLine.length() + 1);
	wcscpy_s(cl.get(), appCmdLine.length() + 1, appCmdLine.c_str());
	BOOL bSuccess = ::CreateProcessAsUserW(hToken, (appName.empty() ? NULL : appName.c_str()),
		cl.get(), NULL, NULL, FALSE, flag, pEnv, NULL, &siex.StartupInfo, &pi);
	pIPC->error = GetLastError();

	// 清理
	CloseHandle(hToken);
	if (pEnv) DestroyEnvironmentBlock(pEnv);
	if (pi.hProcess) CloseHandle(pi.hProcess);
	if (pi.hThread) CloseHandle(pi.hThread);

	// 向调用者报告
	pIPC->status = (bSuccess ? 0x50 : 0x10); // 0x50: 成功, 0x10: 失败
	pIPC->pid = pi.dwProcessId;

	// 结束
	Sleep(1000);
	ReportServiceStatus(SERVICE_STOPPED);
}


void CALLBACK app(
	HWND hwnd,
	HINSTANCE hinst,
	LPTSTR lpCmdLine,
	int nCmdShow
) {
	argc = 0; 
	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!argv || argc < 1) {
		fprintf(stderr, "Failed to parse command line");
		exit(87);
		return;
	}
	w32oop::util::WindowRAIIHelper argvfree([] { LocalFree(argv); });

	// 服务的入口点
	if (argc < 4) exit(87);

	serviceName = argv[2];
	cfgTmpKey = argv[3];
	EnableAllPrivileges(NULL);

	SERVICE_TABLE_ENTRY serviceTable[] = {
		{ const_cast<LPWSTR>(serviceName.c_str()), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ nullptr, nullptr }
	};

	w32oop::util::WindowRAIIHelper servfree([] {
		ServiceManager().get(serviceName).remove();
	}); // 退出时删除服务
	StartServiceCtrlDispatcherW(serviceTable);
}

