#include "GetUIAccessToken.h"
#include <WtsApi32.h>

BOOL EnableAllPrivileges(HANDLE hToken);
HANDLE GetFullUserToken(DWORD sessionId) {
	HANDLE hToken = NULL;
	HANDLE hDupToken = NULL;

	// 1. 获取会话的用户令牌（可能是过滤的令牌）
	if (!WTSQueryUserToken(sessionId, &hToken)) {
		return NULL;
	}

	// 2. 获取令牌的链接令牌（Linked Token）即完整令牌
	TOKEN_LINKED_TOKEN linkedToken{};
	DWORD retLen = 0;
	if (GetTokenInformation(hToken, TokenLinkedToken, &linkedToken, sizeof(linkedToken), &retLen)) {
		// 3. 复制完整令牌（因为链接令牌是只读的）
		if (DuplicateTokenEx(linkedToken.LinkedToken, TOKEN_ALL_ACCESS, NULL,
			SecurityImpersonation, TokenPrimary, &hDupToken)) {
			CloseHandle(linkedToken.LinkedToken);
			CloseHandle(hToken);
			return hDupToken; // 返回完整的用户令牌
		}
		CloseHandle(linkedToken.LinkedToken);
	}

	// 4. 如果没有链接令牌（非管理员用户），则直接复制原令牌
	if (DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, &hDupToken)) {
		CloseHandle(hToken);
		return hDupToken;
	}

	CloseHandle(hToken);
	return NULL;
}

HANDLE __stdcall GetUIAccessToken(DWORD dwSession)
{
	// 参考文档: 
	// https://github.com/killtimer0/uiaccess
	// 
	// “造”一个 token
	HANDLE hTokenActiveUser = NULL, hTokenDupActiveUser = NULL;
	PVOID pEnv = NULL;

	// 获取当前处于活动状态用户的Token
	hTokenActiveUser = GetFullUserToken(dwSession);
	if (!hTokenActiveUser) return NULL;
	// 复制新的Token
	if ((!DuplicateTokenEx(hTokenActiveUser, MAXIMUM_ALLOWED, NULL,
		SecurityImpersonation, TokenPrimary, &hTokenDupActiveUser))
		|| (!hTokenDupActiveUser)) {
		CloseHandle(hTokenActiveUser);
		return NULL;
	}
	CloseHandle(hTokenActiveUser);

	// 设置UIAccess
	BOOL bUIAccess = TRUE;
	if (!SetTokenInformation(hTokenDupActiveUser, TokenUIAccess, &bUIAccess, sizeof(bUIAccess))) {
		CloseHandle(hTokenDupActiveUser);
		return NULL;
	}

	// 启用特权
	EnableAllPrivileges(hTokenDupActiveUser);

	return hTokenDupActiveUser;
}
