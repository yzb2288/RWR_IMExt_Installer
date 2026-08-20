#pragma once
#include <windows.h>
#include <atlstr.h>
#include <atlsimpcoll.h>

namespace MonitorUtils {

    // 屏幕分辨率
    struct ScreenResolution {
        int nWidth;
        int nHeight;

        ScreenResolution() : nWidth(0), nHeight(0) {}
        ScreenResolution(int width, int height) : nWidth(width), nHeight(height) {}

        bool operator==(const ScreenResolution& other) const {
            return (nWidth == other.nWidth && nHeight == other.nHeight);
        }

        bool operator!=(const ScreenResolution& other) const {
            return !operator==(other);
        }
    };

    // 获取主屏幕分辨率（基于显示器当前设置，返回物理像素）
    inline bool GetPrimaryScreenResolution(int& outWidth, int& outHeight) {
        outWidth = 0;
        outHeight = 0;

        DEVMODE devMode;
        ZeroMemory(&devMode, sizeof(devMode));
        devMode.dmSize = sizeof(devMode);

        // NULL 设备名表示主显示器
        if (::EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode)) {
            outWidth = devMode.dmPelsWidth;
            outHeight = devMode.dmPelsHeight;
            return (outWidth > 0 && outHeight > 0);
        }
        return false;
    }

    // 枚举所有屏幕所需的回调上下文
    struct EnumMonitorsContext {
        CSimpleArray<ScreenResolution> resolutionList;
    };

    static BOOL CALLBACK EnumMonitorProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam) {
        EnumMonitorsContext* pContext = reinterpret_cast<EnumMonitorsContext*>(lParam);
        if (pContext == NULL) {
            return FALSE;
        }

        MONITORINFOEX monitorInfo;
        ZeroMemory(&monitorInfo, sizeof(monitorInfo));
        monitorInfo.cbSize = sizeof(monitorInfo);

        if (::GetMonitorInfo(hMonitor, &monitorInfo)) {
            DEVMODE devMode;
            ZeroMemory(&devMode, sizeof(devMode));
            devMode.dmSize = sizeof(devMode);

            // 取该显示器当前分辨率（物理像素）
            if (::EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode) &&
                devMode.dmPelsWidth > 0 && devMode.dmPelsHeight > 0) {

                ScreenResolution resolution(devMode.dmPelsWidth, devMode.dmPelsHeight);
                // 去重：多台相同分辨率的显示器只保留一条
                if (pContext->resolutionList.Find(resolution) == -1) {
                    pContext->resolutionList.Add(resolution);
                }
            }
        }
        return TRUE;
    }

    // 获取所有屏幕分辨率列表（每台显示器当前分辨率，已去重）
    inline bool GetAllScreenResolutionList(CSimpleArray<ScreenResolution>& outResolutionList) {
        outResolutionList.RemoveAll();

        EnumMonitorsContext context;
        ::EnumDisplayMonitors(NULL, NULL, EnumMonitorProc, reinterpret_cast<LPARAM>(&context));

        outResolutionList = context.resolutionList;
        return (outResolutionList.GetSize() > 0);
    }

}
