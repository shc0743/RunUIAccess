#include <Windows.h>
#include <cstdio>
#include <w32use.hpp>
#include "StartUIAccessProcess.h"

using namespace std;

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib,"Rpcrt4.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "wtsapi32.lib")

DWORD SuspendProcess(HANDLE hProcess) {
	typedef DWORD(WINAPI* ZWSUSPENDPROCESS)(HANDLE);
	HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
	if (!hNtdll) return 0;
	ZWSUSPENDPROCESS pNtSuspendProcess = (ZWSUSPENDPROCESS)GetProcAddress(hNtdll, "NtSuspendProcess");
	if (!pNtSuspendProcess) return 0;
	return pNtSuspendProcess(hProcess);
}
DWORD ResumeProcess(HANDLE hProcess) {
	typedef DWORD(WINAPI* ZWRESUMEPROCESS)(HANDLE);
	HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
	if (!hNtdll) return 0;
	ZWRESUMEPROCESS pNtResumeProcess = (ZWRESUMEPROCESS)GetProcAddress(hNtdll, "NtResumeProcess");
	if (!pNtResumeProcess) return 0;
	return pNtResumeProcess(hProcess);
}
bool IsProcessElevated(HANDLE hProcess = GetCurrentProcess());
bool WINAPI IsProcessElevated(HANDLE hProcess) {
	BOOL bElevated = false;
	HANDLE hToken = NULL;
	// Get current process token
	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) return false;
	TOKEN_ELEVATION tokenEle = { 0 };
	DWORD dwRetLen = 0;
	// Retrieve token elevation information
	if (GetTokenInformation(hToken, TokenElevation,
		&tokenEle, sizeof(tokenEle), &dwRetLen)) {
		if (dwRetLen == sizeof(tokenEle)) {
			bElevated = tokenEle.TokenIsElevated;
		}
	}
	CloseHandle(hToken);
	return bElevated;
}

extern HINSTANCE hInst;

// lpCmdLine，别用！会乱码！别用！会乱码！别用！会乱码！
void CALLBACK run(
	HWND hwnd,
	HINSTANCE hinst,
	LPTSTR lpCmdLine,
	int nCmdShow
) {
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!argv || argc < 1) {
		fprintf(stderr, "Failed to parse command line");
		exit(87);
		return;
	}
	w32oop::util::WindowRAIIHelper argvfree([argv] {LocalFree(argv); });

	if (argc < 3) {
		TaskDialog(hwnd, NULL, L"uiaccess.dll Help", L"uiaccess.dll Help",
			L"Usage:\n\t"
			L"rundll32 uiaccess.dll,run <Target Application>"
			, TDCBF_CANCEL_BUTTON, TD_INFORMATION_ICON, NULL
		);
		return;
	}
	
	// 先检查是否已经以管理员身份运行
	if (!IsProcessElevated()) {
		// 提权
		wchar_t winDir[MAX_PATH];
		if (!GetWindowsDirectoryW(winDir, MAX_PATH)) {
			fprintf(stderr, "GetWindowsDirectory failed: %lu", GetLastError());
			exit(-3);
		}

		// 构造rundll32.exe路径
		std::wstring rundll32Path = std::wstring(winDir) + L"\\System32\\rundll32.exe";

		// 获取当前DLL路径
		auto dllPath = std::make_unique<wchar_t[]>(4096);
		if (!GetModuleFileNameW(hInst, dllPath.get(), 4096)) {
			fprintf(stderr, "GetModuleFileName failed: %lu", GetLastError());
			exit(-3);
		}
		// 构建命令行参数
		std::wstring parameters = L"\"" + std::wstring(dllPath.get()) + L"\",run ";
		for (int i = 2; i < argc; ++i) parameters += L"\""s + argv[i] + L"\" ";

		// 执行提权操作
		SHELLEXECUTEINFOW sei = { sizeof(sei) };
		sei.lpVerb = L"runas";
		sei.lpFile = rundll32Path.c_str();
		sei.lpParameters = parameters.c_str();
		sei.hwnd = hwnd;
		sei.nShow = nCmdShow;
		sei.fMask = SEE_MASK_NOCLOSEPROCESS;

		if (!ShellExecuteExW(&sei) || !sei.hProcess) {
			DWORD err = GetLastError();
			if (err == ERROR_CANCELLED) {
				// 用户取消UAC提示
				exit(-ERROR_CANCELLED);
			}
			fprintf(stderr, "ShellExecuteEx failed: %lu", err);
			exit(-3);
		}
		// 等待新进程结束
		WaitForSingleObject(sei.hProcess, INFINITE);
		// 获取退出代码
		DWORD exitCode = 0;
		GetExitCodeProcess(sei.hProcess, &exitCode);
		CloseHandle(sei.hProcess);
		// 退出当前进程
		exit(exitCode);
	}

	// 提取命令行
	wstring cl;
	for (int i = 2; i < argc; ++i) cl += L"\""s + argv[i] + L"\" ";
	// 启动此进程并等待结束
	DWORD pid = 0; DWORD session = 0;
	ProcessIdToSessionId(GetCurrentProcessId(), &session);
	bool ok = StartUIAccessProcess(NULL, cl.c_str(), CREATE_SUSPENDED, &pid, session);
	if (!ok) exit(-1);
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hProcess) exit(-5);
	DWORD code = 1;
	ResumeProcess(hProcess);
	WaitForSingleObject(hProcess, INFINITE);
	GetExitCodeProcess(hProcess, &code);
	CloseHandle(hProcess);
	exit((int)code);
}

