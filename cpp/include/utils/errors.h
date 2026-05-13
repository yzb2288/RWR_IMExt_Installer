#pragma once
#include <windows.h>
#include <atlstr.h>

inline void FastFatalErrorImpl(LPCTSTR message, const char* file, const char* function, int line) {
    CString strLog;
    // 将 char* 转换为 TCHAR 路径并格式化
    strLog.Format(_T("Fatal Error: %s\nFile: %S\nFunction: %S\nLine: %d"), message, file, function, line);
    
    // 立即终止进程，不触发析构函数和常规清理逻辑
    // 退出码 1 表示异常退出
    MessageBox(NULL, strLog, _T("Fatal"), MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
    TerminateProcess(GetCurrentProcess(), 1);
}

#define FastFatalError(msg) FastFatalErrorImpl(msg, __FILE__, __FUNCTION__, __LINE__)