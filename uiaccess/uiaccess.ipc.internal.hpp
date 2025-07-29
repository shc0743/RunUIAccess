#pragma once
#include <Windows.h>

struct StartUIAccessProcess_IPC {
	DWORD cb; // 结构体大小(unused)
	char status; // 0x50: 成功, 0x10: 失败
	char reserved[3]; // 保留字节，确保结构体对齐
	DWORD pid; // 进程ID
	DWORD error; // 错误码
};
