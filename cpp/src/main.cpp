#include "ui/main_window.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

CAppModule _Module;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    HRESULT hRes = ::CoInitialize(NULL);
    ATLASSERT(SUCCEEDED(hRes));

    _Module.Init(NULL, hInstance);

    //AtlInitCommonControls(ICC_TREEVIEW_CLASSES);

    MainWindow wnd;
    int scrWidth = GetSystemMetrics(SM_CXSCREEN);
	int scrHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT wnd_rc = {
        (scrWidth - WND_DEFAULT_WIDTH) / 2,
        (scrHeight - WND_DEFAULT_HEIGHT) / 2,
        (scrWidth + WND_DEFAULT_WIDTH) / 2,
        (scrHeight + WND_DEFAULT_HEIGHT) / 2
    };
    
    HWND hWnd = wnd.Create(NULL, wnd_rc, NULL);

    if (!hWnd)
    {
        wchar_t buf[128];
        wsprintf(buf, L"Create failed: %u", GetLastError());
        MessageBox(NULL, buf, L"Error", 0);
        return 0;
    }

    wnd.ShowWindow(nCmdShow);
    wnd.UpdateWindow();

    // 5. 消息循环
    CMessageLoop theLoop;
    _Module.AddMessageLoop(&theLoop);

    int nRet = theLoop.Run();

    _Module.RemoveMessageLoop();
    _Module.Term();

    ::CoUninitialize();

    return 0;
}