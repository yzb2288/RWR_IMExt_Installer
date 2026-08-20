#pragma once
#include <windows.h>
#include <atlstr.h>
#include "utils/file_utils.h"

namespace RegeditUtils {
    inline bool GetAppCompatFlags(const CString& exePath, CString& outAppCompatFlags) {
        CString exeRealPath = FileUtils::GetRealPath(exePath);
        if (!FileUtils::FileExists(exeRealPath)) {
            return false;
        }

        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers",
            0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            return false; // 键不存在
        }

        wchar_t valueData[1024];
        DWORD dataSize = sizeof(valueData);
        DWORD type = 0;
        // 2. 查询以程序路径为名的值
        LONG result = RegQueryValueExW(hKey, exeRealPath, nullptr, &type, (LPBYTE)valueData, &dataSize);
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS || type != REG_SZ) {
            return false; // 没找到该程序的值，没勾选
        }

        outAppCompatFlags.SetString(valueData);
        return true;
    }

    inline bool SetAppCompatFlags(const CString& exePath, const CString& appCompatFlags) {
        CString exeRealPath = FileUtils::GetRealPath(exePath);
        if (!FileUtils::FileExists(exeRealPath)) {
            return false;
        }

        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers",
            0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
            return false;
        }

        DWORD dataSize = (appCompatFlags.GetLength() + 1) * sizeof(wchar_t);
        LONG result = RegSetValueExW(
            hKey,                           // 键句柄
            exeRealPath,                    // 值名称（完整路径）
            0,                              // 保留
            REG_SZ,                         // 类型
            (const BYTE*)(LPCWSTR)appCompatFlags, // 数据指针
            dataSize                        // 数据字节数
        );

        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
    }


}