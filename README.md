# RunUIAccess

获取 UIAcccess 权限，或者以 UIAccess 权限启动其他进程。

使用 Windows 服务获取 SeTcbPrivilege，而不是“偷”Winlogon的令牌。

参考了 [killtimer0/uiaccess](https://github.com/killtimer0/uiaccess/tree/master) 的一部分代码实现。

# 用法

## 命令行用法

```sh
rundll32 uiaccess.dll,run <目标程序或命令>
```

例如：`rundll32 uiaccess.dll,run notepad.exe`

**注意**需要管理员权限。如果没有权限，命令行会自动尝试请求提权。

## 集成到其他应用程序

DLL 的导出函数*主要*包括：（[完整导出函数](./uiaccess/Source.def)）

```
app                     # 内部使用，请勿手动调用
run                     # 命令行入口函数，只能从 rundll32 调用，请勿在您的程序中调用
GetUIAccessToken        # 获取 UIAccess 的令牌。注意需要 SYSTEM 权限。一般情况下无需调用此函数
StartUIAccessProcess    # 启动 UIAccess 进程。最常用。
StartUIAccessProcessEx  # 启动 UIAccess 进程。(目前和StartUIAccessProcess没有区别。)
IsUIAccess              # 判断当前进程是否以 UIAccess 权限运行。
```

定义：

```cpp
HANDLE WINAPI GetUIAccessToken(DWORD dwSession);
BOOL WINAPI StartUIAccessProcess(LPCWSTR appName, PCWSTR cmdLine, DWORD flag, PDWORD pPid, DWORD dwSession);
BOOL WINAPI StartUIAccessProcessEx(LPCWSTR appName, PCWSTR cmdLine, DWORD flag, PDWORD pPid, DWORD dwSession, DWORD parent_id); // 注意：目前 parent_id 参数无作用
BOOL WINAPI IsUIAccess();
```

**注意**集成版本不会试图自动提权，您需要手动判断权限，或许需要手动请求UAC提权。

# 许可证（LICENSE）

MIT 许可证 （MIT License）
