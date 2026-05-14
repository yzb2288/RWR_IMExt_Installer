#pragma once

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlframe.h>

#include "cjson/cJSON.h"

#include "ui/common.h"
#include "ui/file_tree_view.h"

#include "i18n/i18n.h"

class AskDialog : public CDialogImpl<AskDialog>, public FileTreeView<AskDialog> {
public:
    enum { IDD = IDD_ASK_DIALOG };

    BEGIN_MSG_MAP(AskDialog)
        CHAIN_MSG_MAP(FileTreeView<AskDialog>)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        //MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        COMMAND_ID_HANDLER(IDCANCEL, OnClose)
        COMMAND_ID_HANDLER(IDOK_ASK_DIALOG, OnClose)
        COMMAND_ID_HANDLER(IDCANCEL_ASK_DIALOG, OnClose)
    END_MSG_MAP()

    int m_nSelection; // 记录结果

    AskDialog(LPCTSTR title, LPCTSTR text, LPCTSTR sysIcon, bool askMode = true, const cJSON* const fileRelPathInfo = nullptr, LPCTSTR rwrInstallPath = nullptr, LPCTSTR imextPath = nullptr);
    AskDialog(LPCTSTR title, LPCTSTR text, LPCTSTR sysIcon, bool askMode = true, const CSimpleArray<CString>* const fileRelPathList = nullptr, LPCTSTR rwrInstallPath = nullptr, LPCTSTR imextPath = nullptr);

private:
    CString m_strTitle;
    CString m_strText;
    CString m_strSizeReferText = _T("============SINGLE_LINE_TEXT_SIZE_REFERENCE============");

    int m_gridHorizontalSpacing = 0;
    int m_gridVerticalSpacing = 0;
    SIZE m_sizeReferText = {0, 0};
    SIZE m_sizeText = {0, 0};
    SIZE m_sizeIcon = {0, 0};
    SIZE m_sizeButton = {0, 0};
    int m_btnSpacing = 0;
    int m_minTreeHeight = 0;
    int m_minClientWidth = 0;
    int m_minClientHeight = 0;

    LPCTSTR m_sysIcon;
    bool m_askMode = true;
    bool m_bShowTreeDialog = false;

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSize(UINT, WPARAM, LPARAM lParam, BOOL&);
    //void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);
    LRESULT OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);


    void InitSizeParam();
    void UpdateLayout(int clientWidth, int clientHeight);
    void AdjustWndSize(int expectWidth = 0, int expectHeight = 0);
    void GetMinWinSize(LONG& minWinWidth, LONG& minWinHeight);
    int CalculateStaticHeight(HWND staticText, int nWidth, LPCTSTR lpText);
};

template<typename T = cJSON>
inline bool ShowModalAskDialog(
    HWND hWndParent,
    LPCTSTR title,
    LPCTSTR text,
    const T* const fileRelPathInfo = nullptr,
    LPCTSTR rwrInstallPath = nullptr,
    LPCTSTR imextPath = nullptr
) {
    AskDialog dlg(title, text, IDI_QUESTION, true, fileRelPathInfo, rwrInstallPath, imextPath);
    
    // DoModal 内部会调用 ::EnableWindow(hWndParent, FALSE)
    // 这会导致点击父窗口时产生 Windows 默认的闪烁和拦截音效
    return (int)dlg.DoModal(hWndParent) == IDOK_ASK_DIALOG;
}

template<typename T = cJSON>
inline void ShowModalError(
    HWND hWndParent,
    LPCTSTR title,
    LPCTSTR text,
    const T* const fileRelPathInfo = nullptr,
    LPCTSTR rwrInstallPath = nullptr,
    LPCTSTR imextPath = nullptr
) {
    AskDialog dlg(title, text, IDI_ERROR, false, fileRelPathInfo, rwrInstallPath, imextPath);
    dlg.DoModal(hWndParent);
}

template<typename T = cJSON>
inline void ShowModalWarning(
    HWND hWndParent,
    LPCTSTR title,
    LPCTSTR text,
    const T* const fileRelPathInfo = nullptr,
    LPCTSTR rwrInstallPath = nullptr,
    LPCTSTR imextPath = nullptr
) {
    AskDialog dlg(title, text, IDI_WARNING, false, fileRelPathInfo, rwrInstallPath, imextPath);
    dlg.DoModal(hWndParent);
}

template<typename T = cJSON>
inline void ShowModalInfo(
    HWND hWndParent,
    LPCTSTR title,
    LPCTSTR text,
    const T* const fileRelPathInfo = nullptr,
    LPCTSTR rwrInstallPath = nullptr,
    LPCTSTR imextPath = nullptr
) {
    AskDialog dlg(title, text, IDI_INFORMATION, false, fileRelPathInfo, rwrInstallPath, imextPath);
    dlg.DoModal(hWndParent);
}