#include "IsUIAccess.h"

// 参考了 https://github.com/killtimer0/uiaccess/blob/ad7e671d9472674273acb87049d33c2f482e52f7/uiaccess/uiaccess.c#L107 的实现
BOOL __stdcall IsUIAccess() {
	BOOL result = FALSE;
	HANDLE hToken;
	DWORD dwErr = 0, fUIAccess = 0;
	PDWORD pdwErr = &dwErr, pfUIAccess = &fUIAccess;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		DWORD dwRetLen;

		if (GetTokenInformation(hToken, TokenUIAccess, pfUIAccess, sizeof(*pfUIAccess), &dwRetLen)) {
			result = TRUE;
		}
		else {
			*pdwErr = GetLastError();
		}
		CloseHandle(hToken);
	}
	else {
		*pdwErr = GetLastError();
	}

	return fUIAccess;
}
