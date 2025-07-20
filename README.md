# RunUIAccess

获取 UIAcccess 权限，或者以 UIAccess 权限启动其他进程。

使用 Windows 服务获取 SeTcbPrivilege，而不是“偷”Winlogon的令牌。

参考了 [killtimer0/uiaccess](https://github.com/killtimer0/uiaccess/tree/master) 的一部分代码实现。关于 Windows `UIAccess` 的更多说明也可以前往这里查看，解释得很详细。

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
IsUIAccess              # 判断当前进程是否以 UIAccess 权限运行。
IsProcessElevated       # 判断进程是否以管理员权限运行。
```

定义：

```cpp
HANDLE WINAPI GetUIAccessToken(DWORD dwSession);
BOOL WINAPI StartUIAccessProcess(LPCWSTR appName, PCWSTR cmdLine, DWORD flag, PDWORD pPid, DWORD dwSession);
BOOL WINAPI IsUIAccess();
bool WINAPI IsProcessElevated(HANDLE hProcess);
```

**注意**集成版本不会试图自动提权，您需要手动判断权限，或许需要手动请求UAC提权。

# 已知问题

- `cmd` 对于参数处理和本程序分歧较大，会导致参数处理错误，建议：
- 要么使用 `cmd /c path\to\batchfile.bat` 的形式，不要直接在命令行中编写完整命令
- 要么换用更加现代的 Shell，如 `powershell` (`pwsh`) 等

# 演示程序

[演示程序](./bin/TestApp.exe)

下载演示程序和 [uiaccess.dll](./bin/uiaccess.dll)，放在同一目录下，运行程序，然后尝试打开任务管理器或者按 Win+Tab 即可看到效果。

# 备注

使用框架：[w32oop](https://github.com/shc0743/w32oop) 的部分功能

# 许可证（LICENSE）

MIT 许可证 （MIT License）
