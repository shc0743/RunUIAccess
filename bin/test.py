import ctypes
import sys
from ctypes import wintypes

# 加载uiaccess.dll
uiaccess = ctypes.WinDLL('./uiaccess.dll', use_last_error=True)

# 定义函数原型
IsProcessElevated = uiaccess.IsProcessElevated
IsProcessElevated.argtypes = [wintypes.HANDLE]
IsProcessElevated.restype = wintypes.BOOL

StartUIAccessProcess = uiaccess.StartUIAccessProcess
StartUIAccessProcess.argtypes = [
    wintypes.LPCWSTR,  # lpApplicationName
    wintypes.LPCWSTR,  # lpCommandLine
    wintypes.DWORD,    # flag
    ctypes.POINTER(wintypes.DWORD),  # pPid
    wintypes.DWORD     # dwSession
]
StartUIAccessProcess.restype = wintypes.BOOL

GetLastError = ctypes.windll.kernel32.GetLastError
GetLastError.restype = wintypes.DWORD

# 获取当前会话ID的函数
def get_current_session_id():
    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    
    # 定义函数原型
    GetCurrentProcessId = kernel32.GetCurrentProcessId
    GetCurrentProcessId.argtypes = []
    GetCurrentProcessId.restype = wintypes.DWORD
    
    ProcessIdToSessionId = kernel32.ProcessIdToSessionId
    ProcessIdToSessionId.argtypes = [wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)]
    ProcessIdToSessionId.restype = wintypes.BOOL
    
    # 获取当前进程ID
    pid = GetCurrentProcessId()
    
    # 获取会话ID
    session_id = wintypes.DWORD(0)
    if not ProcessIdToSessionId(pid, ctypes.byref(session_id)):
        raise ctypes.WinError(ctypes.get_last_error())
    
    return session_id.value

def main():
    # 检查管理员权限 (INVALID_HANDLE_VALUE = -1 表示当前进程)
    if not IsProcessElevated(wintypes.HANDLE(-1)):
        print("错误：程序必须以管理员权限运行")
        sys.exit(1)
    
    # 获取用户输入的命令行
    cmd_line = input("请输入要执行的命令（例如: notepad.exe）: ")
    if not cmd_line:
        print("错误：命令行不能为空")
        sys.exit(1)
    
    # 准备参数
    app_name = None  # 设为NULL
    flag = 0         # 默认标志
    pid = wintypes.DWORD(0)
    session_id = get_current_session_id()   # 当前会话
    
    # 调用StartUIAccessProcess
    success = StartUIAccessProcess(
        app_name,
        cmd_line,
        flag,
        ctypes.byref(pid),
        session_id
    )
    
    # 处理结果
    if success:
        print(f"成功启动进程! PID: {pid.value}")
    else:
        print(f"启动失败! {ctypes.WinError(ctypes.get_last_error())}")

if __name__ == "__main__":
    main()
