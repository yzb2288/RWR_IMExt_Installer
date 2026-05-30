#pragma once

#include <atlstr.h>
#include <atlcoll.h>
#include <atltypes.h>

#include "utils/file_utils.h"

class FontResizeTool {
public:
    typedef void (*ResizeProgressCallback)(int progressNum, int totalNum, int ret, void* pCaller);
    enum ResizeProgressStatus {Idle, Resizing, Succeeded, Failed};

    CSimpleArray<CString> m_fontPngFileNameList;
    CSimpleArray<CSize> m_fontPngOldResolutionList;
    CSimpleArray<CSize> m_fontPngNewResolutionList;
    CSimpleArray<int> m_fontPngResizeProgressStatus;

    FontResizeTool(const CString& rwrPath) : m_rwrPath(rwrPath) {m_rwrFontsFolderPath = m_rwrPath + _T("\\") + m_rwrFontsFolderRelPath;}
    bool ScanFonts();
    bool SetTargetResolutionPercentage(int per);
    bool ResizeFonts(ResizeProgressCallback callback = nullptr, void* pCaller = nullptr);

private:
    CString m_rwrPath;
    CString m_rwrFontsFolderRelPath = _T("media\\packages\\vanilla\\fonts");
    CString m_rwrFontsFolderPath;
    CString m_fontPngFileMask = _T("*.png");
    int m_targetResolutionPercentage = 100;
    LONGLONG m_maxPixelNum = 0ll;
    
    bool GetPngSize(const CString& pngPath, CSize& size);
    bool ResizePng(const CString& pngPath, const CSize& newSize, bool overwrite = true);
};