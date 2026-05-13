#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <shlobj.h>
#include <atlstr.h>
#include <atlcoll.h>

#include "utils/timestamp.h"

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "version.lib")

// 单个MD5计算任务回调
typedef void (*progressCallback)(int progressNum, int totalNum, void* pCaller);
typedef void (*failureCallback)(LPCTSTR error, void* pCaller);

// 任务上下文
struct MD5TaskContext {
    const CSimpleArray<CString>* pInputList;  // 输入列表指针
    CSimpleArray<CString>* pOutputList;       // 输出列表指针
    int nIndex;                               // 当前处理的索引
    volatile LONG* pPendingCount;             // 剩余任务计数
    LONG nTotalCount;                         // 任务总数
    progressCallback pProgressCallback;       // 单任务回调
    void* pCaller;                            // 回调调用对象指针
    HANDLE hDoneEvent;                        // 完成信号
};

class FileUtils {
public:
    static CString GetExeDirectoryFile(const CString& strFileName);
    static bool SplitPathFirstComponent(const CString& inFilePath, CString& outFirst, CString& outRest);
    static CStringW NormalizePath(LPCWSTR filePath);
    static CStringA NormalizePath(LPCSTR filePath);
    static CStringW NormalizePathUnix(LPCWSTR filePath);
    static CStringA NormalizePathUnix(LPCSTR filePath);
    static CString GetRealPath(const CString& filePath);
    static CString GetRoamingPath();
    static bool IsFile(const CString& filePath);
    static bool FileExists(const CString& filePath);
    static void OpenFile(const CString& filePath);
    static void ShowInFolder(const CString& filePath);
    static LRESULT SelectFolderInDialog(CString& outFolderPath, const CString& pszTitle, HWND hWnd);
    static CString GetFileMD5(LPCTSTR filePath);
    static VOID CALLBACK Md5WorkCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WORK Work);
    static void BatchCalculateMD5(const CSimpleArray<CString>& filePathList, CSimpleArray<CString>& fileMD5List, progressCallback pProgressCallback = nullptr, void* pCaller = nullptr);
    static LONGLONG GetFileLastWriteUnixTime(LPCTSTR filePath);
    static CString GetVersionCustomTag(LPCTSTR filePath, LPCTSTR keyName);
    static bool Makedirs(LPCTSTR dir);
    static bool CopyFileWithMakedirs(LPCTSTR src, LPCTSTR dest, bool overrideExist = true);
    static bool CopyFilesByFileRelPathWithMakedirs(
        LPCTSTR srcDir, LPCTSTR destDir,
        const CSimpleArray<CString>& fileRelPathList,
        progressCallback pProgressCallback = nullptr, failureCallback pFailureCallback = nullptr, void* pCaller = nullptr,
        bool stopOnFailed = true
    );
};