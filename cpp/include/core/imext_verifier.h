#pragma once
#include <windows.h>
#include <atlstr.h>
#include <atlcoll.h>

#include "cjson/cJSON.h"

#include "i18n/i18n.h"
#include "utils/file_utils.h"
#include "utils/timestamp.h"
#include "utils/semver.h"
extern "C" {
    #include "utils/wildmatch.h"
}
#include "utils/errors.h"

typedef void (*VerifyProgressCallback)(int progressNum, int totalNum, void* pCaller);
typedef void (*ResetVerifyProgressCallback)(void* pCaller);

class ImextVerifier {
public:
    ImextVerifier();
    ~ImextVerifier();
    void Init(
        LPCTSTR installConfJsonPath = nullptr,
        VerifyProgressCallback pVerifyProgressCallback = nullptr,
        ResetVerifyProgressCallback pResetVerifyProgressCallback = nullptr,
        void* pCallbackCaller = nullptr
    );

    CString m_version;
    LONGLONG m_timestamp;
    CString m_targetGameMD5;

    cJSON* m_imextFileRelPathMD5Info = nullptr;
    cJSON* m_imextFileRelPathMD5InfoSorted = nullptr;

    inline static const CString strRwrGameExe = _T("rwr_game.exe");
    inline static const CString strImextDll = _T("IMExt.dll");
    inline static const CString strImextDllVersionTag = _T("FileVersion");
    inline static const CString strImextDllCompatibleGameMD5Tag = _T("CompatibleGameMD5");
    inline static const CString strFirstVersion = _T("1.0.0");
    inline static const CString strFirstVersionImextDllMD5 = _T("b6214e3bb5af8e2979d97e7a31711268");
    inline static const CString strFirstVersionRwrGameExeMD5 = _T("7a0615b323f780a8a16bf34ea4c2e9ab");

    bool CheckGameCompatibility(const CString& gameFolderPath);
    bool CheckInstallVersion(const CString& gameFolderPath, CString& imextVer);
    int VersionCompare(const CString& compareVersion);
    const cJSON* CheckFileMD5(const CString& gameFolderPath); // 返回合并的JSON节点

private:
    CString m_installConfJsonPath;
    cJSON* m_installConf = nullptr;
    cJSON* m_fileMask = nullptr;
    cJSON* m_gameFileRelPathMD5Info = nullptr;

    VerifyProgressCallback m_pVerifyProgressCallback;
    ResetVerifyProgressCallback m_pResetVerifyProgressCallback;
    void* m_pCallbackCaller;

    void LoadInstallConfJson();
    void InitCheckFile();
    void InitSortFile();
    void SortFileRelPathInfo(const cJSON* const fileRelPathInfo, cJSON* const sortedFileRelPathInfo);
    void AddFileRelPathInfoWithCategory(
        cJSON* const sortedFileRelPathInfo,
        const char* categoryName,
        const char* fileRelPath,
        cJSON* fileRelPathInfoItem,
        bool referenceAdd = false,
        bool duplicateAdd = false
    );
    void AddCategoriesWithOrder(cJSON* const sortedFileRelPathInfo, const cJSON* const refFileMaskCategories);
    void RemoveEmptyCategories(cJSON* const sortedFileRelPathInfo);
    void GetFileMD5List(const CSimpleArray<CString>& filePathList, CSimpleArray<CString>& fileMD5List);
};