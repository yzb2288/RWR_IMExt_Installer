#include "core/imext_verifier.h"

ImextVerifier::ImextVerifier() {
}

ImextVerifier::~ImextVerifier() {
    cJSON_Delete(m_installConf);
}

void ImextVerifier::Init(LPCTSTR installConfJsonPath, VerifyProgressCallback pVerifyProgressCallback, ResetVerifyProgressCallback pResetVerifyProgressCallback, void* pCallbackCaller) {
    m_installConfJsonPath = installConfJsonPath;
    m_pVerifyProgressCallback = pVerifyProgressCallback;
    m_pResetVerifyProgressCallback = pResetVerifyProgressCallback;
    m_pCallbackCaller = pCallbackCaller;

    if (!installConfJsonPath || m_installConfJsonPath.GetLength() == 0) {
        m_installConfJsonPath = _T("./install_conf.json");
    }

    if (!FileUtils::FileExists(m_installConfJsonPath)) {
        CString errorMsg;
        errorMsg.Format(_T("Install conf json \"%s\" not found!"), m_installConfJsonPath);
        FastFatalError(errorMsg);
    }

    LoadInstallConfJson();
    InitCheckFile();
    InitSortFile();
}

void ImextVerifier::LoadInstallConfJson() {
    FILE* fp = NULL;
    if (_wfopen_s(&fp, m_installConfJsonPath, L"rb") != 0 || !fp) {
        CString errorMsg;
        errorMsg.Format(_T("Failed to open \"%s\" !"), m_installConfJsonPath);
        FastFatalError(errorMsg);
    }

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* data = (char*)malloc(length + 1);
    fread(data, 1, length, fp);
    data[length] = '\0';
    fclose(fp);

    m_installConf = cJSON_Parse(data);

    if (!m_installConf) {
        const char *error_ptr = cJSON_GetErrorPtr();
        free(data);
        CString errorMsg;
        errorMsg.Format(_T("Failed to parse \"%s\" ! %s"), m_installConfJsonPath, CA2T(error_ptr, CP_UTF8));
        FastFatalError(errorMsg);
    }

    if (!cJSON_IsObject(m_installConf)) {
        free(data);
        CString errorMsg;
        errorMsg.Format(_T("Install conf json \"%s\" is not object type json!"), m_installConfJsonPath);
        FastFatalError(errorMsg);
    }

    free(data);
}

void ImextVerifier::InitCheckFile() {
    cJSON* version = cJSON_GetObjectItemCaseSensitive(m_installConf, _KU8(IDS_KEY_VERSION));
    if (!cJSON_IsString(version)) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" not found!"), CA2T(_KU8(IDS_KEY_VERSION), CP_UTF8));
        FastFatalError(errorMsg);
    }
    m_version = CA2T(version->valuestring, CP_UTF8);

    cJSON* timestamp = cJSON_GetObjectItemCaseSensitive(m_installConf, _KU8(IDS_KEY_TIMESTAMP));
    if (!cJSON_IsString(timestamp)) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" not found!"), CA2T(_KU8(IDS_KEY_TIMESTAMP), CP_UTF8));
        FastFatalError(errorMsg);
    }
    m_timestampStr = CA2T(timestamp->valuestring, CP_UTF8);
    m_timestamp = ParseIsoToTimestamp(m_timestampStr);

    cJSON* lastGameUpdateVersion = cJSON_GetObjectItemCaseSensitive(m_installConf, _KU8(IDS_KEY_GAME_VERSION));
    if (!cJSON_IsString(lastGameUpdateVersion)) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" not found!"), CA2T(_KU8(IDS_KEY_GAME_VERSION), CP_UTF8));
        FastFatalError(errorMsg);
    }
    m_lastGameUpdateVersion = CA2T(lastGameUpdateVersion->valuestring, CP_UTF8);

    cJSON* lastGameUpdateTimestamp = cJSON_GetObjectItemCaseSensitive(m_installConf, _KU8(IDS_KEY_LAST_GAME_UPDATE_TIMESTAMP));
    if (!cJSON_IsString(lastGameUpdateTimestamp)) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" not found!"), CA2T(_KU8(IDS_KEY_LAST_GAME_UPDATE_TIMESTAMP), CP_UTF8));
        FastFatalError(errorMsg);
    }
    m_lastGameUpdateTimestampStr = CA2T(lastGameUpdateTimestamp->valuestring, CP_UTF8);
    m_lastGameUpdateTimestamp = ParseIsoToTimestamp(m_lastGameUpdateTimestampStr);

    cJSON* targetGameMD5 = cJSON_GetObjectItemCaseSensitive(m_installConf, _KU8(IDS_KEY_TARGET_GAME_MD5));
    if (!cJSON_IsString(targetGameMD5)) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" not found!"), CA2T(_KU8(IDS_KEY_TARGET_GAME_MD5), CP_UTF8));
        FastFatalError(errorMsg);
    }
    if (!targetGameMD5->valuestring || strlen(targetGameMD5->valuestring) != 32) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" doesn't have md5 string!"), CA2T(_KU8(IDS_KEY_TARGET_GAME_MD5), CP_UTF8));
        FastFatalError(errorMsg);
    }
    m_targetGameMD5 = CA2T(targetGameMD5->valuestring, CP_UTF8);

    m_fileMask = cJSON_GetObjectItemCaseSensitive(m_installConf, _KU8(IDS_KEY_FILE_MASK));
    if (!cJSON_IsObject(m_fileMask)) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" not found!"), CA2T(_KU8(IDS_KEY_FILE_MASK), CP_UTF8));
        FastFatalError(errorMsg);
    }

    m_imextFileRelPathMD5Info = cJSON_GetObjectItemCaseSensitive(m_installConf, _KU8(IDS_KEY_IMEXT_FILE_MD5));
    if (!cJSON_IsObject(m_imextFileRelPathMD5Info)) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" not found!"), CA2T(_KU8(IDS_KEY_IMEXT_FILE_MD5), CP_UTF8));
        FastFatalError(errorMsg);
    }

    m_gameFileRelPathMD5Info = cJSON_GetObjectItemCaseSensitive(m_installConf, _KU8(IDS_KEY_GAME_FILE_MD5));
    if (!cJSON_IsObject(m_gameFileRelPathMD5Info)) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" not found!"), CA2T(_KU8(IDS_KEY_GAME_FILE_MD5), CP_UTF8));
        FastFatalError(errorMsg);
    }
}

void ImextVerifier::InitSortFile() {
    cJSON_DeleteItemFromObjectCaseSensitive(m_installConf, _KU8(IDS_KEY_IMEXT_FILE_MD5_SORTED));
    m_imextFileRelPathMD5InfoSorted = cJSON_AddObjectToObject(m_installConf, _KU8(IDS_KEY_IMEXT_FILE_MD5_SORTED));

    SortFileRelPathInfo(m_imextFileRelPathMD5Info, m_imextFileRelPathMD5InfoSorted);
}

void ImextVerifier::SortFileRelPathInfo(const cJSON* const fileRelPathInfo, cJSON* const sortedFileRelPathInfo) {
    if (!fileRelPathInfo || !sortedFileRelPathInfo) return;

    if(!cJSON_IsObject(fileRelPathInfo)) {
        FastFatalError(_T("fileRelPathInfo is not object!"));
    }

    if(!cJSON_IsObject(sortedFileRelPathInfo)) {
        FastFatalError(_T("sortedFileRelPathInfo is not object!"));
    }

    // 确保清空sortedFileRelPathInfo
    cJSON *childToDelete = sortedFileRelPathInfo->child;
    while (childToDelete) {
        cJSON *next = childToDelete->next;
        cJSON_Delete(childToDelete);
        childToDelete = next;
    }
    sortedFileRelPathInfo->child = NULL;
    
    //先按顺序添加空的category, 包括Others
    AddCategoriesWithOrder(sortedFileRelPathInfo, m_fileMask);

    cJSON* childfileRelPath = fileRelPathInfo->child;
    while (childfileRelPath)
    {
        CString fileRelPath(CA2T(childfileRelPath->string, CP_UTF8));
        if (!cJSON_IsString(childfileRelPath) || !childfileRelPath->valuestring || strlen(childfileRelPath->valuestring) == 0) {
            CString errorMsg;
            errorMsg.Format(_T("Key \"%s\" doesn't have hash string!"), fileRelPath);
            FastFatalError(errorMsg);
        }

        bool bMatch = false;
        cJSON* childCategory = m_fileMask->child;
        while (childCategory)
        {
            CString categoryName(CA2T(childCategory->string, CP_UTF8));
            if (!cJSON_IsArray(childCategory)) {
                CString errorMsg;
                errorMsg.Format(_T("Key \"%s\" is not array!"), categoryName);
                FastFatalError(errorMsg);
            }

            cJSON *categoryFileMask = nullptr;
            cJSON_ArrayForEach(categoryFileMask, childCategory) {
                // 确保当前项是字符串
                if (cJSON_IsString(categoryFileMask) && categoryFileMask->valuestring) {
                    // glob匹配
                    CStringA pattern(FileUtils::NormalizePathUnix(categoryFileMask->valuestring));
                    CStringA text(FileUtils::NormalizePathUnix(childfileRelPath->string));
                    if (wildmatch(pattern, text, WILD_CASEFOLD | WILD_PATHNAME)) {
                        bMatch = true;
                        // 添加到对应分类节点, 并退出cJSON_ArrayForEach
                        AddFileRelPathInfoWithCategory(sortedFileRelPathInfo, childCategory->string, childfileRelPath->string, childfileRelPath, true);
                        break;
                    }

                } else {
                    CString errorMsg;
                    errorMsg.Format(_T("Key \"%s\" has non-string file mask!"), categoryName);
                    FastFatalError(errorMsg);
                }
            }

            if (bMatch) {
                break;
            }
            childCategory = childCategory->next;
        }

        if (!bMatch) {
            // 添加到Others分类
            AddFileRelPathInfoWithCategory(sortedFileRelPathInfo, _KU8(IDS_KEY_CATEGORY_OTHERS), childfileRelPath->string, childfileRelPath, true);
        }
        
        childfileRelPath = childfileRelPath->next;
    }

    //最后删除空的category
    RemoveEmptyCategories(sortedFileRelPathInfo);
}

void ImextVerifier::AddFileRelPathInfoWithCategory(cJSON* const sortedFileRelPathInfo, const char* categoryName, const char* fileRelPath, cJSON* fileRelPathInfoItem, bool referenceAdd, bool duplicateAdd) {
    if (!sortedFileRelPathInfo || !categoryName || !fileRelPath || !fileRelPathInfoItem) return;

    cJSON* category = cJSON_GetObjectItemCaseSensitive(sortedFileRelPathInfo, categoryName);
    if (!cJSON_IsObject(category)) {
        // 极端情况下不保证顺序添加category
        cJSON_DeleteItemFromObjectCaseSensitive(sortedFileRelPathInfo, categoryName);

        //需要排序创建
        category = cJSON_AddObjectToObject(sortedFileRelPathInfo, categoryName);
    }

    cJSON_DeleteItemFromObjectCaseSensitive(category, fileRelPath);

    if (referenceAdd) {
        cJSON_AddItemReferenceToObject(category, fileRelPath, fileRelPathInfoItem);
    } else {
        if (duplicateAdd) {
            cJSON_AddItemToObject(category, fileRelPath, cJSON_Duplicate(fileRelPathInfoItem, true));
        } else {
            cJSON_AddItemToObject(category, fileRelPath, fileRelPathInfoItem);
        }
    }
}

void ImextVerifier::AddCategoriesWithOrder(cJSON* const sortedFileRelPathInfo, const cJSON* const refFileMaskCategories) {
    if (!sortedFileRelPathInfo || !refFileMaskCategories) return;

    cJSON* childCategory = refFileMaskCategories->child;
    while (childCategory) {
        cJSON_DeleteItemFromObjectCaseSensitive(sortedFileRelPathInfo, childCategory->string);
        cJSON_AddObjectToObject(sortedFileRelPathInfo, childCategory->string);
        childCategory = childCategory->next;
    }

    cJSON* othersCategory = cJSON_GetObjectItemCaseSensitive(sortedFileRelPathInfo, _KU8(IDS_KEY_CATEGORY_OTHERS));
    if (!cJSON_IsObject(othersCategory)) {
        cJSON_DeleteItemFromObjectCaseSensitive(sortedFileRelPathInfo, _KU8(IDS_KEY_CATEGORY_OTHERS));
        othersCategory = cJSON_AddObjectToObject(sortedFileRelPathInfo, _KU8(IDS_KEY_CATEGORY_OTHERS));
    }
}

void ImextVerifier::RemoveEmptyCategories(cJSON* const sortedFileRelPathInfo) {
    if (!sortedFileRelPathInfo) return;

    cJSON* childCategory = sortedFileRelPathInfo->child;
    while (childCategory) {
        cJSON *next = childCategory->next;

        if (!cJSON_IsObject(childCategory) || !childCategory->child) {
            cJSON_DeleteItemFromObjectCaseSensitive(sortedFileRelPathInfo, childCategory->string);
        }
        
        childCategory = next;
    }
}

const cJSON* ImextVerifier::CheckFileMD5(const CString& gameFolderPath) {
    cJSON_DeleteItemFromObjectCaseSensitive(m_installConf, _KU8(IDS_KEY_CHECK_RESULT));
    cJSON* checkResult = cJSON_AddObjectToObject(m_installConf, _KU8(IDS_KEY_CHECK_RESULT));

    cJSON* missFileSorted = cJSON_AddObjectToObject(checkResult, _KU8(IDS_KEY_CHECK_RESULT_MISS_FILE));
    cJSON* diffFileSorted = cJSON_AddObjectToObject(checkResult, _KU8(IDS_KEY_CHECK_RESULT_DIFF_FILE));
    cJSON* origFileSorted = cJSON_AddObjectToObject(checkResult, _KU8(IDS_KEY_CHECK_RESULT_ORIG_FILE));
    cJSON* sameFileSorted = cJSON_AddObjectToObject(checkResult, _KU8(IDS_KEY_CHECK_RESULT_SAME_FILE));

    AddCategoriesWithOrder(missFileSorted, m_fileMask);
    AddCategoriesWithOrder(diffFileSorted, m_fileMask);
    AddCategoriesWithOrder(origFileSorted, m_fileMask);
    AddCategoriesWithOrder(sameFileSorted, m_fileMask);

    if (!m_imextFileRelPathMD5InfoSorted) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" not found!"), CA2T(_KU8(IDS_KEY_IMEXT_FILE_MD5_SORTED), CP_UTF8));
        FastFatalError(errorMsg);
    }

    cJSON* childCategory = m_imextFileRelPathMD5InfoSorted->child;
    while (childCategory) {
        CString categoryName(CA2T(childCategory->string, CP_UTF8));
        if (!cJSON_IsObject(childCategory)) {
            CString errorMsg;
            errorMsg.Format(_T("Key \"%s\" is not object!"), categoryName);
            FastFatalError(errorMsg);
        }

        CSimpleArray<CString> filePathList;
        cJSON* childfileRelPath = childCategory->child;
        while (childfileRelPath) {
            CString fileRelPath(CA2T(childfileRelPath->string, CP_UTF8));
            if (!cJSON_IsString(childfileRelPath) || !childfileRelPath->valuestring || strlen(childfileRelPath->valuestring) != 32) {
                CString errorMsg;
                errorMsg.Format(_T("Key \"%s\" doesn't have md5 string!"), fileRelPath);
                FastFatalError(errorMsg);
            }
            filePathList.Add(gameFolderPath + _T("\\") + fileRelPath);
            childfileRelPath = childfileRelPath->next;
        }

        CSimpleArray<CString> fileMD5List;
        GetFileMD5List(filePathList, fileMD5List);

        childfileRelPath = childCategory->child;
        int index = 0;
        while (childfileRelPath && index < fileMD5List.GetSize()) {
            bool bCurrentGameHasFile = false;
            bool bImextSame = false;
            bool bGameSame = false;
            CString fileRelPath(CA2T(childfileRelPath->string, CP_UTF8));
            if (fileMD5List[index].GetLength() == 32) {
                bCurrentGameHasFile = true;

                // 只有当前游戏目录下存在这个文件才比较
                CString imextFileMD5(CA2T(childfileRelPath->valuestring, CP_UTF8));
                if (fileMD5List[index].CompareNoCase(imextFileMD5) == 0) {
                    bImextSame = true;
                }

                cJSON* gameFileMD5Info = cJSON_GetObjectItemCaseSensitive(m_gameFileRelPathMD5Info, childfileRelPath->string);
                if (cJSON_IsString(gameFileMD5Info) && gameFileMD5Info->valuestring && strlen(gameFileMD5Info->valuestring) == 32) {
                    CString gameFileMD5(CA2T(gameFileMD5Info->valuestring, CP_UTF8));
                    if (fileMD5List[index].CompareNoCase(gameFileMD5) == 0) {
                        bGameSame = true;
                    }
                }
            }

            // 添加到对应的结果和分类
            if (!bCurrentGameHasFile) { //miss
                AddFileRelPathInfoWithCategory(missFileSorted, childCategory->string, childfileRelPath->string, cJSON_CreateNull(), false, false);
            } else {
                if (!bImextSame && !bGameSame) { //diff
                    LONGLONG currentGameFileMTime = FileUtils::GetFileLastWriteUnixTime(gameFolderPath + _T("\\") + fileRelPath);
                    if (currentGameFileMTime > m_timestamp) {
                        cJSON* note = cJSON_CreateString(CT2A(_TR(IDS_NEWER_MODIFY_TIMESTAMP), CP_UTF8)); // 不复制的情况下不用在这里清理
                        AddFileRelPathInfoWithCategory(diffFileSorted, childCategory->string, childfileRelPath->string, note, false, false);
                    } else {
                        AddFileRelPathInfoWithCategory(diffFileSorted, childCategory->string, childfileRelPath->string, cJSON_CreateNull(), false, false);
                    }
                    

                } else {
                    if (bGameSame) { //orig
                        AddFileRelPathInfoWithCategory(origFileSorted, childCategory->string, childfileRelPath->string, cJSON_CreateNull(), false, false);
                    }
                    if (bImextSame) { //same
                        AddFileRelPathInfoWithCategory(sameFileSorted, childCategory->string, childfileRelPath->string, cJSON_CreateNull(), false, false);
                    }
                }
            }
            
            index += 1;
            childfileRelPath = childfileRelPath->next;
        }

        childCategory = childCategory->next;
    }

    RemoveEmptyCategories(missFileSorted);
    RemoveEmptyCategories(diffFileSorted);
    RemoveEmptyCategories(origFileSorted);
    RemoveEmptyCategories(sameFileSorted);

    return checkResult;
}

void ImextVerifier::GetFileMD5List(const CSimpleArray<CString>& filePathList, CSimpleArray<CString>& fileMD5List) {
    if (m_pResetVerifyProgressCallback) {
        m_pResetVerifyProgressCallback(m_pCallbackCaller);
    }
    FileUtils::BatchCalculateMD5(filePathList, fileMD5List, [] (int progressNum, int totalNum, void* pCaller) {
        if (pCaller) {
            ImextVerifier* pThis = static_cast<ImextVerifier*>(pCaller);
            if (pThis->m_pVerifyProgressCallback) {
                pThis->m_pVerifyProgressCallback(progressNum, totalNum, pThis->m_pCallbackCaller);
            }
        }
    }, this);
}

int ImextVerifier::CheckGameCompatibility(const CString& gameFolderPath) {
    CString gameExeMD5 = FileUtils::GetFileMD5(gameFolderPath + _T("\\") + strRwrGameExe);
    if (gameExeMD5.CompareNoCase(m_targetGameMD5) == 0) {
        return GameCompatibility::TargetOriginalExe;
    } else {
        cJSON* imextGameExeMD5Info = cJSON_GetObjectItemCaseSensitive(m_imextFileRelPathMD5Info, CT2A(strRwrGameExe, CP_UTF8));
        if (!cJSON_IsString(imextGameExeMD5Info) || !imextGameExeMD5Info->valuestring || strlen(imextGameExeMD5Info->valuestring) != 32) {
            CString errorMsg;
            errorMsg.Format(_T("Key \"%s\" doesn't have md5 string!"), strRwrGameExe);
            FastFatalError(errorMsg);
        }
        CString imextGameExeMD5(CA2T(imextGameExeMD5Info->valuestring, CP_UTF8));
        if (gameExeMD5.CompareNoCase(imextGameExeMD5) == 0) {
            return GameCompatibility::IMExtCurrentExe;
        } else {
            CString gameExePatchedTimestampString = FileUtils::GetVersionCustomTag(gameFolderPath + _T("\\") + strRwrGameExe, strGamePatchedTimestampTag);
            if (gameExePatchedTimestampString.IsEmpty()) {
                if (gameExeMD5.CompareNoCase(strFirstVersionRwrGameExeMD5) == 0) {
                    // 当游戏EXE和第一版插件发布的游戏EXE相同时, 也认为是带Tag的
                    return GameCompatibility::IMExtTagExe;
                } else {
                    // 可能是游戏更新、被其他软件修改、早期内测版本
                    return GameCompatibility::UnknownExe;
                }
            } else {
                // 通过检测gameExePatchedTimestampString判断是否是旧版本
                return GameCompatibility::IMExtTagExe;
            }
        }
    }
}

bool ImextVerifier::CheckInstallVersion(const CString& gameFolderPath, CString& imextVer) {
    imextVer.SetString(_T(""));

    // 检查IMExt Resources目录下的所有文件是否都存在, 如果有一个不存在则算没有安装, 如果后续升级增加或者减少了文件则另外写一份V1版的资源判断逻辑
    cJSON* imextResources = cJSON_GetObjectItemCaseSensitive(m_imextFileRelPathMD5InfoSorted, _KU8(IDS_KEY_CATEGORY_IMEXT_RESOURCES));
    if (!cJSON_IsObject(imextResources)) {
        CString errorMsg;
        errorMsg.Format(_T("Key \"%s\" doesn't exist!"), CA2T(_KU8(IDS_KEY_CATEGORY_IMEXT_RESOURCES), CP_UTF8));
        FastFatalError(errorMsg);
    }
    cJSON* childfileRelPath = imextResources->child;
    while (childfileRelPath)
    {
        CString fileRelPath(CA2T(childfileRelPath->string, CP_UTF8));
        if (!cJSON_IsString(childfileRelPath) || !childfileRelPath->valuestring || strlen(childfileRelPath->valuestring) != 32) {
            CString errorMsg;
            errorMsg.Format(_T("Key \"%s\" doesn't have md5 string!"), fileRelPath);
            FastFatalError(errorMsg);
        }
        if (!FileUtils::IsFile(gameFolderPath + _T("\\") + fileRelPath)) {
            return false;
        }
        childfileRelPath = childfileRelPath->next;
    }

    CString imextDllPath(gameFolderPath + _T("\\") + strImextDll);
    imextVer = FileUtils::GetVersionCustomTag(imextDllPath, strImextDllVersionTag);
    CString imextCompatibleGameMD5 = FileUtils::GetVersionCustomTag(imextDllPath, strImextDllCompatibleGameMD5Tag);
    CString gameExeMD5 = FileUtils::GetFileMD5(gameFolderPath + _T("\\") + strRwrGameExe);

    if (imextVer.IsEmpty()) {
        if (FileUtils::GetFileMD5(imextDllPath).CompareNoCase(strFirstVersionImextDllMD5) == 0) {
            imextVer.SetString(strFirstVersion);
            if (gameExeMD5.CompareNoCase(strFirstVersionRwrGameExeMD5) == 0) {
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        if (gameExeMD5.CompareNoCase(imextCompatibleGameMD5) == 0) {
            return true;
        } else {
            return false;
        }
    }
}

LONGLONG ImextVerifier::CheckInstallTimestamp(const CString& gameFolderPath) {
    // 只检查IMExt.dll的编译时间
    CString imextPatchedTimestampString = FileUtils::GetVersionCustomTag(gameFolderPath + _T("\\") + strImextDll, strGamePatchedTimestampTag);
    if (imextPatchedTimestampString.IsEmpty()) {
        // 是第一次发布版或者是未发现时间戳Tag的版本统一当成第一次发布版的时间戳
        return llFirstVersionImextTimestamp;
    } else {
        LONGLONG timestamp = ParseIsoToTimestamp(imextPatchedTimestampString);
        if (timestamp == 0) {
            CString errorMsg;
            errorMsg.Format(_T("Invalid IMExt Timestamp Tag: %s"), imextPatchedTimestampString);
            FastFatalError(errorMsg);
        }
        return timestamp;
    }
}

int ImextVerifier::VersionCompare(const CString& compareVersion) {
    semver_t current_version = {};
    semver_t compare_version = {};

    if (semver_parse(CT2A(m_version, CP_UTF8), &current_version)
    || semver_parse(CT2A(compareVersion, CP_UTF8), &compare_version)) {
        CString errorMsg;
        errorMsg.Format(_T("Invalid semver string!\ncompare: %s\ncurrent: %s"), compareVersion, m_version);
        FastFatalError(errorMsg);
    }

    int resolution = semver_compare(compare_version, current_version);

    // Free allocated memory when we're done
    semver_free(&current_version);
    semver_free(&compare_version);

    return resolution;
}