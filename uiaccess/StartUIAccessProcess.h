#pragma once
/*
MIT License, Copyright (c) 2025 @chcs1013
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
/// 启动带有 UIAccess 权限的进程。
/// </summary>
/// <param name="appName">同 CreateProcess 的 lpApplicationName。可以为 NULL</param>
/// <param name="cmdLine">命令行</param>
/// <param name="flag">创建标志</param>
/// <param name="pPid">接收进程PID。可以为 NULL</param>
/// <param name="dwSession">目标会话</param>
/// <returns>操作是否成功</returns>
BOOL WINAPI StartUIAccessProcess(LPCWSTR appName, PCWSTR cmdLine, DWORD flag, PDWORD pPid, DWORD dwSession);

/// <summary>
/// 启动带有 UIAccess 权限的进程，以指定进程为父进程。
/// </summary>
/// <param name="appName">同 CreateProcess 的 lpApplicationName。可以为 NULL</param>
/// <param name="cmdLine">命令行</param>
/// <param name="flag">创建标志</param>
/// <param name="pPid">接收进程PID。可以为 NULL</param>
/// <param name="dwSession">目标会话</param>
/// <param name="parent_id">父进程 PID</param>
/// <returns>操作是否成功</returns>
BOOL WINAPI StartUIAccessProcessEx(LPCWSTR appName, PCWSTR cmdLine, DWORD flag, PDWORD pPid, DWORD dwSession, DWORD parent_id);

#ifdef __cplusplus
}
#endif
