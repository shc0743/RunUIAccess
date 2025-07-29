#include <w32use.hpp>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;

class WindowTopMostDemo : public Window {
public:
	WindowTopMostDemo() : Window(L"窗口置顶测试", 400, 160, 0, 0, WS_OVERLAPPEDWINDOW) {}
private:
	CheckBox isTopMost;
	Button runit;
	Static description;
	wstring appPath;
	HMODULE hDll = NULL;
	void onCreated() override {
		WCHAR app[1024]{};
		GetModuleFileNameW(GetModuleHandleW(NULL), app, 1024);
		appPath = app;
		SetCurrentDirectoryW((appPath + L"\\..").c_str());

		isTopMost.set_parent(this);
		isTopMost.create(L"置顶此窗口", 360, 20, 10, 10);
		isTopMost.onChanged([this](EventData&) {
			set_topmost(isTopMost.checked());
		});
		runit.set_parent(this);
		runit.create(L"以 UIAccess 运行", 360, 30, 10, 40);

		// 先试图加载 DLL
		hDll = LoadLibraryW((appPath + L"/../uiaccess.dll").c_str());
		if (!hDll) {
			isTopMost.text(L"置顶此窗口 -- (UIAccess 不可用: 无法加载 DLL)");
			runit.enable(false);
		}
		else {
			typedef BOOL(WINAPI* IsUIAccess_t)();
			auto IsUIAccess = (IsUIAccess_t)GetProcAddress(hDll, "IsUIAccess");
			if (IsUIAccess) {
				if (IsUIAccess()) {
					isTopMost.text(L"置顶此窗口 ++");
					text(L"窗口置顶测试++ [UIAccess]"); // this的text
				}
				else {
					isTopMost.text(L"置顶此窗口 -");
				}
			}
			else {
				FreeLibrary(hDll);
				hDll = NULL;
				isTopMost.text(L"置顶此窗口 -- (UIAccess 不可用: DLL 损坏)");
				runit.enable(false);
			}
		}

		runit.onClick([this](EventData&) {
			if (!hDll) return;
			typedef bool (WINAPI*IsProcessElevated_t)(HANDLE);
			typedef BOOL(WINAPI* StartUIAccessProcess_t)(LPCWSTR appName, PCWSTR cmdLine, DWORD flag, PDWORD pPid, DWORD dwSession);
			auto IsProcessElevated = (IsProcessElevated_t)GetProcAddress(hDll, "IsProcessElevated");
			auto StartUIAccessProcess = (StartUIAccessProcess_t)GetProcAddress(hDll, "StartUIAccessProcess");
			if (!IsProcessElevated || !StartUIAccessProcess) {
				MessageBoxW(hwnd, L"错误: DLL 损坏", L"错误", MB_ICONERROR);
				return;
			}
			if (!IsProcessElevated(GetCurrentProcess())) {
				// 偷懒 >_<
				if (int((LONG_PTR)ShellExecuteW(hwnd, L"runas", L"rundll32.exe",
					(L"\"" + appPath + L"/../uiaccess.dll\",run \""s + appPath + L"\"").c_str(), NULL, 1)
				) > 32) close();
				return;
			}
			DWORD pid = GetCurrentProcessId(), session = 0;
			ProcessIdToSessionId(pid, &session); // 重要！
			if (!StartUIAccessProcess(appPath.c_str(), L"app", 0, NULL, session)) {
				MessageBoxW(hwnd, format(L"错误: 启动 UIAccess 进程失败！\n\n错误详情: {}",
					w32oop::util::ErrorChecker().message()).c_str(), L"错误", MB_ICONERROR);
				return;
			}
			close();
		});
		description.set_parent(this);
		description.create(L"也可以通过把其他应用程序拖到此窗口来启动它们。", 360, 20, 10, 80);

		DragAcceptFiles(hwnd, 1);
	}
	void onDestroy() {
		if (hDll) FreeLibrary(hDll);
	}
protected:
	void OnDropFiles(EventData& event) {
		HDROP hDrop = reinterpret_cast<HDROP>(event.wParam);
		UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);

		if (fileCount == 0) return;

		if (fileCount > 1) {
			MessageBoxW(hwnd, L"一次只能运行一个文件", L"错误", MB_ICONERROR);
			return;
		}

		WCHAR currentAppPath[1024]{};
		GetModuleFileNameW(nullptr, currentAppPath, 1024);

		wstring dllPath = wstring(currentAppPath) + L"/../uiaccess.dll";

		for (UINT i = 0; i < fileCount; ++i)
		{
			WCHAR filePath[MAX_PATH]{};
			DragQueryFileW(hDrop, i, filePath, MAX_PATH);

			wstring cmdLine = L"\"" + dllPath + L"\",run \"" + filePath + L"\"";
			ShellExecuteW(hwnd, L"runas", L"rundll32.exe", cmdLine.c_str(), nullptr, SW_SHOW);
		}

		DragFinish(hDrop);
	}
	virtual void setup_event_handlers() override {
		WINDOW_add_handler(WM_DROPFILES, OnDropFiles);
	}
};

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PWSTR pCmdLine,
	_In_ int nCmdShow)
{
	WindowTopMostDemo window;
	window.create();
	window.set_main_window();
	window.center();
	window.show();
	return window.run();
}