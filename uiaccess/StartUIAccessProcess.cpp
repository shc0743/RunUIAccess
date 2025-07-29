#include <w32use.hpp>
#include "StartUIAccessProcess.h"
#include "uiaccess.ipc.internal.hpp"

using namespace std;

extern HINSTANCE hInst;
std::wstring GenerateUUIDW();

BOOL __stdcall StartUIAccessProcess(
	LPCWSTR appName, PCWSTR cmdLine, DWORD flag, PDWORD pPid, DWORD dwSession
) {
	// 通过创建服务来提权
	//使用file mapping
	wstring kn = GenerateUUIDW();
	DWORD errCode = -1;
	try {
		w32FileHandle hFileMapping = CreateFileMappingW(
			INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE | SEC_COMMIT, 0, 4096,
			(L"Global\\" + kn).c_str()
		);
		auto pIPC = (StartUIAccessProcess_IPC*)MapViewOfFile(
			hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(StartUIAccessProcess_IPC)
		);
		memset(pIPC, 0, sizeof(StartUIAccessProcess_IPC));
		w32oop::util::RAIIHelper _([&pIPC] {
			UnmapViewOfFile(pIPC);
		});
		// 首先设置注册表
		RegistryKey root(HKEY_LOCAL_MACHINE, L"SOFTWARE");
		RegistryKey data = root.create(kn);
		// 存储相关信息
		data.set(L"callerPID", RegistryValue(GetCurrentProcessId(), REG_DWORD));
		if (appName) data.set(L"targetAppName", RegistryValue(wstring(appName), REG_SZ));
		data.set(L"targetCmdLine", RegistryValue(wstring(cmdLine), REG_SZ));
		data.set(L"flag", RegistryValue(flag, REG_DWORD));
		data.set(L"session", RegistryValue(dwSession, REG_DWORD));
		// 创建服务以提权
		ServiceManager scm;
		auto dllPath = make_unique<wchar_t[]>(4096);
		GetModuleFileNameW(hInst, dllPath.get(), 4096);
		wstring mycl = L"rundll32.exe \""s + dllPath.get() + L"\",app Elevator-" + kn + L" " + kn;
		Service sc = scm.create(L"Elevator-" + kn, mycl, SERVICE_DEMAND_START);
		sc.start();
		time_t now = time(0);
		while (1) {
			if (time(0) - now > 10) {
				sc.remove();
				errCode = ERROR_TIMEOUT;
				throw exception("Timed-out waiting for service");
			}
			Sleep(100);
			// 检查是否成功
			if (pIPC->status == 0x50) break;
			if (pIPC->status == 0x10) {
				errCode = pIPC->error;
				throw exception("Operation failed");
			}
		}
		// 接收进程信息
		DWORD pid = pIPC->pid;
		// 检查是否成功？
		if (!pid) {
			// 不成功
			errCode = pIPC->error;
			throw exception("Unsuccessful operation");
		}
		// 成功！
		if (pPid) *pPid = pid;
		errCode = 0;
	}
	catch (exception& exc) {
		if (errCode == -1) errCode = GetLastError();
		fprintf(stderr, "Error: %s\n", exc.what());
		try {
			RegistryKey(HKEY_LOCAL_MACHINE, L"SOFTWARE").delete_key(kn); // 清理
		}
		catch (...) {}
		SetLastError(errCode);
		return FALSE;
	}
	SetLastError(errCode);
	return TRUE;
}

std::wstring GenerateUUIDW() {
	std::wstring guid;
	UUID uuid;
	if (RPC_S_OK != UuidCreate(&uuid)) return guid;
	wchar_t tmp[37 * 2] = { 0 };
	wsprintf(tmp, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		uuid.Data1, uuid.Data2, uuid.Data3,
		uuid.Data4[0], uuid.Data4[1],
		uuid.Data4[2], uuid.Data4[3],
		uuid.Data4[4], uuid.Data4[5],
		uuid.Data4[6], uuid.Data4[7]);
	guid.assign(tmp);
	return guid;
}

