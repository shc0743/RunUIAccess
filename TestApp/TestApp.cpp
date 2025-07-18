#include <w32use.hpp>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;

class WindowTopMostDemo : public Window {
public:
	WindowTopMostDemo() : Window(L"窗口置顶测试", 400, 120, 0, 0, WS_OVERLAPPEDWINDOW) {}
private:
	CheckBox isTopMost;
	Button runit;
	void onCreated() override {
		isTopMost.set_parent(this);
		isTopMost.create(L"置顶此窗口", 360, 20, 10, 10);
		isTopMost.onChanged([this](EventData&) {
			set_topmost(isTopMost.checked());
		});
		runit.set_parent(this);
		runit.create(L"以 UIAccess 运行", 360, 30, 10, 40);
		runit.onClick([this](EventData&) {
			WCHAR app[1024]{};
			GetModuleFileNameW(GetModuleHandleW(NULL), app, 1024);
			if (int((LONG_PTR)ShellExecuteW(hwnd, L"open", L"rundll32.exe",
				(L"uiaccess.dll,run \""s + app + L"\"").c_str(), NULL, 1)
			) > 32) close();
		});
	}
protected:
	virtual void setup_event_handlers() override {

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