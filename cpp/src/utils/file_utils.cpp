#include "utils/file_utils.h"

CString FileUtils::GetExeDirectoryFile(const CString& strFileName) {
    TCHAR szPath[MAX_PATH];
    ::GetModuleFileName(NULL, szPath, MAX_PATH);
    CString strPath(szPath);
    int pos = strPath.ReverseFind(_T('\\'));
    if (pos != -1) {
        strPath = strPath.Left(pos + 1); // 保留最后的反斜杠
    }
    return strPath + strFileName;
}

bool FileUtils::SplitPathFirstComponent(const CString& inFilePath, CString& outFirst, CString& outRest) {
    CString copyFilePath(inFilePath);
    copyFilePath.Replace(_T('/'), _T('\\'));

    const TCHAR* pszNext = PathFindNextComponent(copyFilePath);

    if (pszNext) {
        // 计算第一个部分的长度 (pszNext 指向下一个斜杠后的字符)
        // 所以要减去 1 得到斜杠位置

        if (*pszNext == '\0')
        {
            outFirst.SetString(copyFilePath);
            outRest.SetString(pszNext);
        }
        else
        {
            size_t len = (pszNext - 1) - copyFilePath;
            outFirst.SetString(copyFilePath, (int)len);
            outRest.SetString(pszNext);
        }
        return true;
    }

    return false;
}

CStringW FileUtils::NormalizePath(LPCWSTR filePath) {
    CStringW copyFilePath(filePath);
    if (copyFilePath.IsEmpty()) {
        return copyFilePath;
    }

    copyFilePath.Replace(L'/', L'\\');
    
    // 循环将 "\\" 替换为 "\"
    while (copyFilePath.Replace(L"\\\\", L"\\") > 0) {
        // Replace 返回替换的次数，只要大于 0 就继续，直到没有连续反斜杠为止
    }

    if (copyFilePath[copyFilePath.GetLength() - 1] == L'\\') {
        copyFilePath.Truncate(copyFilePath.GetLength() - 1);
    }
    
    return copyFilePath;
}

CStringA FileUtils::NormalizePath(LPCSTR filePath) {
    CStringA copyFilePath(filePath);
    if (copyFilePath.IsEmpty()) {
        return copyFilePath;
    }

    copyFilePath.Replace('/', '\\');
    
    // 循环将 "\\" 替换为 "\"
    while (copyFilePath.Replace("\\\\", "\\") > 0) {
        // Replace 返回替换的次数，只要大于 0 就继续，直到没有连续反斜杠为止
    }

    if (copyFilePath[copyFilePath.GetLength() - 1] == '\\') {
        copyFilePath.Truncate(copyFilePath.GetLength() - 1);
    }
    
    return copyFilePath;
}

CStringW FileUtils::NormalizePathUnix(LPCWSTR filePath) {
    CStringW copyFilePath(filePath);
    if (copyFilePath.IsEmpty()) {
        return copyFilePath;
    }

    copyFilePath.Replace(L'\\', L'/');
    
    // 循环将 "\\" 替换为 "\"
    while (copyFilePath.Replace(L"//", L"/") > 0) {
        // Replace 返回替换的次数，只要大于 0 就继续，直到没有连续斜杠为止
    }

    if (copyFilePath[copyFilePath.GetLength() - 1] == L'/') {
        copyFilePath.Truncate(copyFilePath.GetLength() - 1);
    }
    
    return copyFilePath;
}

CStringA FileUtils::NormalizePathUnix(LPCSTR filePath) {
    CStringA copyFilePath(filePath);
    if (copyFilePath.IsEmpty()) {
        return copyFilePath;
    }

    copyFilePath.Replace('\\', '/');
    
    // 循环将 "\\" 替换为 "\"
    while (copyFilePath.Replace("//", "/") > 0) {
        // Replace 返回替换的次数，只要大于 0 就继续，直到没有连续斜杠为止
    }

    if (copyFilePath[copyFilePath.GetLength() - 1] == '/') {
        copyFilePath.Truncate(copyFilePath.GetLength() - 1);
    }
    
    return copyFilePath;
}

CString FileUtils::GetRealPath(const CString& filePath) {
    CString realPath;
    DWORD bufferLength = GetFullPathName(filePath, 0, NULL, NULL);

    if (bufferLength > 0) {
        LPTSTR pBuffer = realPath.GetBuffer(bufferLength);
        GetFullPathName(filePath, bufferLength, pBuffer, NULL);
        realPath.ReleaseBuffer();
    }

    return realPath;
}

// 返回相对路径会带有./或者../的前缀
bool FileUtils::GetRelativePath(const CString& filePath, bool isFileFolder, const CString& fromPath, bool isFromFolder, CString& relPath) {
    // 基础防错检查
    if (filePath.IsEmpty() || fromPath.IsEmpty())
    {
        relPath = filePath;
        return false;
    }

    // 3. 调用 API 计算相对路径
    TCHAR szRelative[MAX_PATH] = { 0 };
    BOOL bRet = ::PathRelativePathTo(
        szRelative, 
        NormalizePath(fromPath), 
        isFromFolder ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL, 
        NormalizePath(filePath), 
        isFileFolder ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL
    );

    if (bRet) {
        relPath = szRelative;
        return true;
    } else {
        // 计算失败（例如 C 盘到 D 盘），将原始路径赋给输出变量并返回 false
        relPath = filePath;
        return false;
    }
}

bool FileUtils::IsFile(const CString& filePath) {
    DWORD dwAttrib = GetFileAttributes(filePath);

    // 检查返回值是否有效，且确保它不是一个目录
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
           !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool FileUtils::FileExists(const CString& filePath) {
    DWORD dwAttrib = GetFileAttributes(filePath);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES);
}

void FileUtils::OpenFile(const CString& filePath) {
    ShellExecute(NULL, L"open", filePath, NULL, NULL, SW_SHOW);
}

void FileUtils::ShowInFolder(const CString& filePath) {
    CString param;
    param.Format(L"/select,\"%s\"", filePath);
    ShellExecute(NULL, L"open", L"explorer.exe", param, NULL, SW_SHOW);
}

LRESULT FileUtils::SelectFolderInDialog(CString& outFolderPath, const CString& pszTitle, HWND hWnd)
{
    IFileOpenDialog* pFileOpen = nullptr;

    // 1. 创建对话框实例
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
                                IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr)) {
        // 2. 设置为文件夹选择模式
        FILEOPENDIALOGOPTIONS options;
        pFileOpen->GetOptions(&options);
        pFileOpen->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        pFileOpen->SetTitle(pszTitle);

        // 3. 显示对话框 (m_hWnd 是你 WTL 窗口的句柄)
        hr = pFileOpen->Show(hWnd);

        if (SUCCEEDED(hr)) {
            // 4. 获取选中的文件夹路径
            IShellItem* pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr)) {
                    // 将路径设置到你的 Edit 控件中
                    outFolderPath = pszFilePath;
                }
                CoTaskMemFree(pszFilePath); // 释放内存
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    return hr;
}

CString FileUtils::GetFileMD5(LPCTSTR filePath) {
    CString strResult = _T("");
    HANDLE hFile = CreateFile(NormalizePath(filePath), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return strResult;

    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    DWORD cbHashObject = 0, cbHash = 0, cbRead = 0;
    PBYTE pbHashObject = NULL;
    BYTE rgbHash[16]; // MD5 16 字节

    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, NULL, 0))) {
        DWORD cbData = 0;
        BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0);
        pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHashObject);

        if (pbHashObject && BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0))) {
            BYTE buffer[8192]; // 8KB 缓冲区提高读取效率
            while (ReadFile(hFile, buffer, sizeof(buffer), &cbRead, NULL) && cbRead > 0) {
                BCryptHashData(hHash, buffer, cbRead, 0);
            }
            if (BCRYPT_SUCCESS(BCryptFinishHash(hHash, rgbHash, sizeof(rgbHash), 0))) {
                for (int i = 0; i < 16; i++) {
                    CString strTmp;
                    strTmp.Format(_T("%02x"), rgbHash[i]);
                    strResult += strTmp;
                }
            }
            BCryptDestroyHash(hHash);
        }
        if (pbHashObject) HeapFree(GetProcessHeap(), 0, pbHashObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    CloseHandle(hFile);
    return strResult;
}

VOID CALLBACK FileUtils::Md5WorkCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WORK Work) {
    MD5TaskContext* pTask = static_cast<MD5TaskContext*>(Context);
    
    // 按照索引获取路径，计算 MD5
    CString strPath = (*pTask->pInputList)[pTask->nIndex];
    CString strMD5 = GetFileMD5(strPath);
    
    // 关键：直接写入输出列表对应的位置
    // CSimpleArray 的操作在多线程下读写不同索引是安全的
    (*pTask->pOutputList)[pTask->nIndex] = strMD5;
    
    LONG remainCount = InterlockedDecrement(pTask->pPendingCount);

    if (pTask->pProgressCallback) {
        pTask->pProgressCallback((pTask->nTotalCount - remainCount), pTask->nTotalCount, pTask->pCaller);
    }

    // 任务计数递减
    if (remainCount == 0) {
        SetEvent(pTask->hDoneEvent);
    }
}

void FileUtils::BatchCalculateMD5(const CSimpleArray<CString>& filePathList, CSimpleArray<CString>& fileMD5List, progressCallback pProgressCallback, void* pCaller) {
    int count = filePathList.GetSize();
    fileMD5List.RemoveAll();
    if (count == 0) return;

    // 1. 预先占位，确保索引可用
    for (int i = 0; i < count; i++) {
        fileMD5List.Add(_T("")); 
    }

    HANDLE hAllDone = CreateEvent(NULL, TRUE, FALSE, NULL);
    volatile LONG pendingCount = count;

    // 2. 分配上下文和工作项
    MD5TaskContext* contexts = new MD5TaskContext[count];
    PTP_WORK* workItems = new PTP_WORK[count];

    for (int i = 0; i < count; i++) {
        contexts[i].pInputList = &filePathList;
        contexts[i].pOutputList = &fileMD5List;
        contexts[i].nIndex = i;
        contexts[i].pPendingCount = &pendingCount;
        contexts[i].nTotalCount = count;
        contexts[i].pProgressCallback = pProgressCallback;
        contexts[i].pCaller = pCaller;
        contexts[i].hDoneEvent = hAllDone;

        workItems[i] = CreateThreadpoolWork(Md5WorkCallback, &contexts[i], NULL);
        if (workItems[i]) {
            SubmitThreadpoolWork(workItems[i]);
        } else {
            InterlockedDecrement(&pendingCount);
        }
    }

    // 3. 等待所有任务完成
    WaitForSingleObject(hAllDone, INFINITE);

    // 4. 清理资源
    for (int i = 0; i < count; i++) {
        if (workItems[i]) CloseThreadpoolWork(workItems[i]);
    }

    delete[] contexts;
    delete[] workItems;
    CloseHandle(hAllDone);
}

LONGLONG FileUtils::GetFileLastWriteUnixTime(LPCTSTR filePath) {
    // 使用 FILE_SHARE_READ 确保文件被占用时也能读取属性
    HANDLE hFile = CreateFile(NormalizePath(filePath), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE) return 0;

    FILETIME ftWrite;
    LONGLONG unixTime = 0;

    if (GetFileTime(hFile, NULL, NULL, &ftWrite)) {
        unixTime = FileTimeToUnixTime(ftWrite);
    }

    CloseHandle(hFile);
    return unixTime;
}

CString FileUtils::GetVersionCustomTag(LPCTSTR filePath, LPCTSTR keyName) {
    CString nFilePath = NormalizePath(filePath);
    CString strResult = _T("");

    if (!IsFile(nFilePath)) return strResult;

    // 1. 获取版本信息大小
    DWORD dwHandle = 0;
    DWORD dwSize = GetFileVersionInfoSize(nFilePath, &dwHandle);
    if (dwSize == 0) return strResult;

    struct TRANSLATION {
        WORD wLanguage;
        WORD wCodePage;
    } *pTrans = NULL;
    UINT uTransLen = 0;

    // 2. 分配缓冲区并获取数据
    BYTE* pBuffer = new BYTE[dwSize];
    if (GetFileVersionInfo(nFilePath, dwHandle, dwSize, pBuffer)) {
        // 先查语言代码
        if (VerQueryValue(pBuffer, _T("\\VarFileInfo\\Translation"), (LPVOID*)&pTrans, &uTransLen)) {
            if (uTransLen > 0 && pTrans != NULL) {
                CString strSubBlock;
                // 根据实际发现的语言/编码构造路径
                strSubBlock.Format(_T("\\StringFileInfo\\%04x%04x\\%s"), 
                                pTrans[0].wLanguage, pTrans[0].wCodePage, keyName);
                
                // ... 然后再执行 VerQueryValue ...
                TCHAR* szValue = NULL;
                UINT uLen = 0;

                // 4. 查询特定的键值
                if (VerQueryValue(pBuffer, strSubBlock, (LPVOID*)&szValue, &uLen)) {
                    if (uLen > 0 && szValue != NULL) {
                        strResult = szValue;
                    }
                }
            }
        }
    }

    delete[] pBuffer;
    return strResult;
}

bool FileUtils::Makedirs(LPCTSTR dir) {
    DWORD dwAttrib = GetFileAttributes(dir);
    if (dwAttrib != INVALID_FILE_ATTRIBUTES) {
        if (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
            return false;
        }
        return true;
    }

    int ret = ::SHCreateDirectoryEx(NULL, dir, NULL);
    
    if (ret == ERROR_SUCCESS || ret == ERROR_ALREADY_EXISTS) {
        return true;
    }
    
    return false;
}

bool FileUtils::CopyFileWithMakedirs(LPCTSTR src, LPCTSTR dest, bool overrideExist) {
    // 1. 提取目標路徑的父文件夾
    TCHAR szPath[MAX_PATH];
    _tcscpy_s(szPath, MAX_PATH, dest);
    ::PathRemoveFileSpec(szPath); // 需要包含 <shlwapi.h> 並鏈接 shlwapi.lib

    // 2. 遞歸創建目標文件夾
    if (!Makedirs(szPath)) {
        return false;
    }

    // 3. 複製文件
    // COPY_FILE_COPY_SYMLINK: 如果是符號鏈接則複製鏈接本身
    // CopyFileEx 默認會保留源文件的時間戳（創建、修改、訪問時間）和文件屬性
    BOOL bRet = ::CopyFileEx(
        src, 
        dest, 
        NULL,           // 進度回調（不需要）
        NULL,           // 回調參數
        NULL,           // 可用於取消複製的布爾值
        COPY_FILE_ALLOW_DECRYPTED_DESTINATION | (overrideExist ? 0 : COPY_FILE_FAIL_IF_EXISTS) // 標誌位
    );

    return bRet ? true : false;
}

bool FileUtils::CopyFilesByFileRelPathWithMakedirs(
    LPCTSTR srcDir, LPCTSTR destDir,
    const CSimpleArray<CString>& fileRelPathList,
    progressCallback pProgressCallback, failureCallback pFailureCallback, void* pCaller,
    bool stopOnFailed
) {
    int count = fileRelPathList.GetSize();
    if (count == 0) return false;
    bool finalRet = true;
    for (int i = 0; i < count; i++) {
        if (pProgressCallback) {
            pProgressCallback(i + 1, count, pCaller);
        }
        CString srcFile = NormalizePath(CString(srcDir) + _T("\\") + fileRelPathList[i]);
        if (FileExists(srcFile)) {
            CString destFile = NormalizePath(CString(destDir) + _T("\\") + fileRelPathList[i]);
            bool ret = CopyFileWithMakedirs(srcFile, destFile, true);
            if (!ret) {
                if (pFailureCallback) {
                    CString errorMsg;
                    errorMsg.Format(_T("Copy file failed!\nsrc: %s\ndest: %s"), srcFile, destFile);
                    pFailureCallback(errorMsg, pCaller);
                }
                if (stopOnFailed) {
                    return false;
                } else {
                    finalRet = false;
                }
            }
        }
    }
    return finalRet;
}

CString FileUtils::GetRoamingPath() {
    TCHAR szPath[MAX_PATH];
    // CSIDL_APPDATA 对应 Roaming 目录
    if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, FALSE)) {
        return CString(szPath);
    }
    return _T("");
}

bool FileUtils::GetFileFullPathListByMask(LPCTSTR filePath, LPCTSTR relFileMask, CSimpleArray<CString>& outputFileFullPathList, bool recursive, bool findFolder) {
    bool ret = GetFileRelPathListByMask(filePath, relFileMask, outputFileFullPathList, recursive, findFolder);

    int count = outputFileFullPathList.GetSize();
    for (int i = 0; i < count; i++) {
        outputFileFullPathList[i] = NormalizePath(CString(filePath) + _T("\\") + outputFileFullPathList[i]);
    }

    return ret;
}

bool FileUtils::GetFileRelPathListByMask(LPCTSTR filePath, LPCTSTR relFileMask, CSimpleArray<CString>& outputFileRelPathList, bool recursive, bool findFolder) {
    outputFileRelPathList.RemoveAll();
    CStringA pattern(CT2A(FileUtils::NormalizePathUnix(relFileMask), CP_UTF8));
    CString nFilePath = NormalizePath(filePath);

    // 检查输入路径是否有效
    if (!FileExists(nFilePath)) {
        return false;
    }

    // 如果是搜索路径是文件直接返回false
    if (IsFile(nFilePath)) {
        return false;
    }

    // 如果是文件夹则循环扫描
    CAtlList<CString> searchRelFolderPathQueue;
    searchRelFolderPathQueue.AddTail(_T(""));
    // 由于采用了wildmatch, 这里直接写一个全部匹配的通配符
    CString searchMask = _T("*");

    bool bSuccess = true;
    while (!searchRelFolderPathQueue.IsEmpty()) {
        CString searchRelFolderPath = searchRelFolderPathQueue.RemoveHead();
        CString searchFolderPath = NormalizePath(nFilePath + _T("\\") + searchRelFolderPath);
        
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(searchFolderPath + _T("\\") + searchMask, &findData);
        if (hFind == INVALID_HANDLE_VALUE) {
            bSuccess = false;
            continue;
        }

        do {
            // 跳过当前目录 "." 和上级目录 ".."
            if (_tcscmp(findData.cFileName, _T(".")) == 0 || _tcscmp(findData.cFileName, _T("..")) == 0) {
                continue;
            }

            // 构造当前相对路径
            CString relPath;
            if (searchRelFolderPath.IsEmpty()) {
                relPath = findData.cFileName;
            } else {
                relPath = searchRelFolderPath + _T("\\") + findData.cFileName;
            }

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // 如果是文件夹且开启了递归, 则添加子文件夹到搜索队列
                if (recursive) {
                    searchRelFolderPathQueue.AddTail(relPath);
                }
                // 输出文件夹路径模式
                if (findFolder) {
                    // 根据相对路径是否匹配加入结果列表
                    CStringA text(CT2A(FileUtils::NormalizePathUnix(relPath), CP_UTF8));
                    if (wildmatch(pattern, text, WILD_CASEFOLD | WILD_PATHNAME)) {
                        outputFileRelPathList.Add(relPath);
                    }
                }
            } else {
                if (!findFolder) {
                    // 根据相对路径是否匹配加入结果列表
                    CStringA text(CT2A(FileUtils::NormalizePathUnix(relPath), CP_UTF8));
                    if (wildmatch(pattern, text, WILD_CASEFOLD | WILD_PATHNAME)) {
                        outputFileRelPathList.Add(relPath);
                    }
                }
            }
        } while (FindNextFile(hFind, &findData));

        FindClose(hFind);
    }

    return bSuccess;
}
