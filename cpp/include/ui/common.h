#pragma once
#include <windows.h>

#include "resource.h"

// 控件间距参数
#define GRID_HORIZONTAL_SPACING         8
#define GRID_VERTICAL_SPACING           8

// 主窗口大小
#define WND_DEFAULT_HEIGHT              300
#define WND_DEFAULT_WIDTH               430

// 主窗口按钮尺寸Padding参数
#define BTN_TEXT_VERTICAL_PADDING       5
#define BTN_TEXT_HORIZONTAL_PADDING     10

// 消息
#define WM_USER_UPDATE_TREE_MAIN       (WM_USER + 100)
#define WM_USER_UPDATE_TREE_DIALOG     (WM_USER + 200)

// 颜色
#define MY_COLOR_DEFAULT RGB(0, 0, 0)
#define MY_COLOR_ERROR RGB(255, 0, 0)
#define MY_COLOR_CRITICAL RGB(255, 69, 0)
#define MY_COLOR_WARNING RGB(218, 165, 32)
#define MY_COLOR_SUCCESS RGB(0, 128, 0)
#define MY_COLOR_SUCCESS_DARK RGB(0, 100, 0)

/*
// 已经定义到resource.h, 数值量会自动保留
// 主窗口控件ID
#define IDC_BTN_SELECT_RWR_INSTALL_PATH 1001
#define IDT_UPDATE_STATIC_TIMER         1001
#define IDC_EDIT_RWR_INSTALL_PATH       1002
#define IDC_BTN_SELECT_BACKUP_PATH      1003
#define IDC_EDIT_BACKUP_PATH            1004
#define IDC_BTN_INSTALL_IMEXT           1005
#define IDC_LABEL_IMEXT_INSTALL_STATUS  1006
#define IDC_TREE_MAIN                   1007
#define IDC_LABEL_PROGRESS              1008

// 主窗口UI更新定时器
#define IDT_UPDATE_STATIC_TIMER         301
*/

// DPI 缩放像素间距
inline int ScalePixelForWindow(HWND hWnd, int logicalPixels)
{
    UINT dpi = 96; // 默认标准 DPI
    HMODULE hUser32 = GetModuleHandle(TEXT("user32.dll"));
    if (hUser32) {
        typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
        PFN_GetDpiForWindow pGetDpiForWindow = (PFN_GetDpiForWindow)GetProcAddress(hUser32, "GetDpiForWindow");
        if (pGetDpiForWindow) {
            dpi = pGetDpiForWindow(hWnd);
        } else {
            // 兼容 Win7/Win8 系统的老旧方法
            HDC hdc = ::GetDC(hWnd);
            if (hdc) {
                dpi = GetDeviceCaps(hdc, LOGPIXELSY);
                ::ReleaseDC(hWnd, hdc);
            }
        }
    }
    // 计算缩放后的像素（带四舍五入）
    return MulDiv(logicalPixels, dpi, 96);
}