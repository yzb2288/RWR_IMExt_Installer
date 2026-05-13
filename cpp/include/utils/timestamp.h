#pragma once
#include <windows.h>
#include <oleauto.h>
#include <atlbase.h>
#include <atlstr.h>

#pragma comment(lib, "OleAut32.lib")

inline LONGLONG FileTimeToUnixTime(FILETIME& ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    // 5. 应用时区补偿并转为 Unix 时间戳
    // Windows 基准时间是 1601-01-01，Unix 是 1970-01-01
    static const LONGLONG DIFF_TO_UNIX = 116444736000000000LL;
    LONGLONG unixTime = (uli.QuadPart - DIFF_TO_UNIX) / 10000000LL;

    return unixTime;
}

// 将 ISO8601 (2026-03-13T19:44:43.835341+08:00) 转为 Unix 时间戳
inline LONGLONG ParseIsoToTimestamp(LPCTSTR szIsoStr) {

    CString str(szIsoStr);
    if (str.GetLength() < 19) return 0;

    // 1. 处理 'T' 分隔符：替换为空格以适配 WinAPI
    str.Replace(_T('T'), _T(' '));

    // 2. 提取并处理时区部分 (+08:00)
    long tzOffsetMinutes = 0;
    int nPlus = str.Find(_T('+'));
    int nMinus = (nPlus == -1) ? str.Find(_T('-'), 11) : -1; // 跳过日期里的减号
    int nTzStart = (nPlus != -1) ? nPlus : nMinus;

    if (nTzStart != -1) {
        CString strTz = str.Mid(nTzStart); // 例如 "+08:00"
        if (strTz.GetLength() >= 6) {
            int h = _ttoi(strTz.Mid(1, 2));
            int m = _ttoi(strTz.Mid(4, 2));
            tzOffsetMinutes = h * 60 + m;
            // 如果是 +08:00，表示本地比 UTC 快，轉 UTC 需減去偏移
            if (strTz[0] == _T('+')) tzOffsetMinutes = -tzOffsetMinutes;
        }
    }

    // 3. 去除微妙 (.835341)
    // 寻找秒之后的点号
    int nDot = str.Find(_T('.'));
    CString strCleanTime;
    if (nDot != -1) {
        // 只保留点号之前的内容: "2026-03-13 19:44:43"
        strCleanTime = str.Left(nDot);
    }
    else if (nTzStart != -1) {
        // 如果没有点号但有时区, 保留时区前的内容
        strCleanTime = str.Left(nTzStart);
    }
    else {
        // 没有点号没有时区, 直接等于这个字符串
        strCleanTime = str;
    }
    strCleanTime.Trim();

    // 4. 使用 OLE 自动化解析剩余的时间部分
    DATE dt;
    // 使用 LOCALE_INVARIANT 避免不同语言系统差异
    HRESULT hr = VarDateFromStr(CT2W(strCleanTime), LOCALE_INVARIANT, 0, &dt);
    if (FAILED(hr)) return 0;

    // 5. 转换为 Windows FILETIME (UTC)
    SYSTEMTIME st;
    VariantTimeToSystemTime(dt, &st);

    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);

    // 4. 转换为 unixTime (UTC)
    LONGLONG unixTime = FileTimeToUnixTime(ft);

    // 加上偏移量分钟 (tzOffsetMinutes 已经在上面根据正负号取反了)
    unixTime += (long long)(tzOffsetMinutes * 60);

    return unixTime;
}

inline BOOL UnixTimeToSystemTime(LONGLONG unixTime, SYSTEMTIME& st) {
    // 1. Unix Epoch 與 Windows Epoch 之間的差值 (以 100 奈秒為單位)
    // 116444736000000000 是從 1601-01-01 到 1970-01-01 的 100ns 數
    LONGLONG ll = (unixTime * 10000000LL) + 116444736000000000LL;

    // 2. 將計算後的 64 位整數轉為 FILETIME
    FILETIME ft;
    ft.dwLowDateTime = (DWORD)ll;
    ft.dwHighDateTime = (DWORD)(ll >> 32);

    // 3. 將 FILETIME 轉換為 SYSTEMTIME
    // 注意：這裡得到的 st 是 UTC 時間
    return ::FileTimeToSystemTime(&ft, &st);
}

inline BOOL UnixTimeToLocalSystemTime(LONGLONG unixTime, SYSTEMTIME& stLocal) {
    SYSTEMTIME stUtc;
    if (!UnixTimeToSystemTime(unixTime, stUtc)) {
        return FALSE;
    }
    
    // 將 UTC 轉換為當前系統設置的本地時區時間
    return ::SystemTimeToTzSpecificLocalTime(NULL, &stUtc, &stLocal);
}

inline CString GetFormattedTime(LPCTSTR pszFormat = _T("%04d-%02d-%02d %02d:%02d:%02d")){
    SYSTEMTIME st;
    // 獲取當前本地時間
    ::GetLocalTime(&st);

    CString strTime;
    // 使用 CString 的 Format 方法，按照格式化字符串進行拼接
    // %04d 表示 4 位數字，不足補 0；%02d 表示 2 位數字，不足補 0
    strTime.Format(pszFormat, 
        st.wYear, st.wMonth, st.wDay, 
        st.wHour, st.wMinute, st.wSecond);

    return strTime;
}

inline CString GetFormattedTime(SYSTEMTIME& st, LPCTSTR pszFormat = _T("%04d-%02d-%02d %02d:%02d:%02d")){
    CString strTime;
    // 使用 CString 的 Format 方法，按照格式化字符串進行拼接
    // %04d 表示 4 位數字，不足補 0；%02d 表示 2 位數字，不足補 0
    strTime.Format(pszFormat, 
        st.wYear, st.wMonth, st.wDay, 
        st.wHour, st.wMinute, st.wSecond);

    return strTime;
}