#pragma once
#include <windows.h>
#include <shlobj.h>

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlstr.h>
#include <atlcrack.h>

#include "cjson/cJSON.h"

#include "ui/common.h"
#include "ui/file_tree_view.h"
#include "ui/ask_dialog.h"
#include "ui/my_edit.h"
#include "ui/color_button.h"
#include "ui/font_resize_tool_dialog.h"

#include "i18n/i18n.h"
#include "utils/file_utils.h"
#include "core/imext_verifier.h"
#include "core/rwr_config.h"

struct LabelDrawInfo {
    CString text;
    COLORREF color;
};

typedef CWinTraits<WS_OVERLAPPEDWINDOW | WS_VISIBLE, WS_EX_APPWINDOW | WS_EX_COMPOSITED> CMainTraits;

class MainWindow : public CWindowImpl<MainWindow, CWindow, CMainTraits>, public FileTreeView<MainWindow>
{
public:

    DECLARE_WND_CLASS_EX(L"RwrImextInstallerApp", CS_HREDRAW | CS_VREDRAW, COLOR_WINDOW)

    BEGIN_MSG_MAP(MainWindow)
        CHAIN_MSG_MAP(FileTreeView<MainWindow>)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_LBUTTONDOWN(OnLButtonDown)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic) // 用于改颜色
        MESSAGE_HANDLER(WM_USER_UPDATE_TREE_MAIN, OnUpdateTreeMain)
        COMMAND_ID_HANDLER(IDC_BTN_SELECT_RWR_INSTALL_PATH, OnSelectRwrInstallPath)
        COMMAND_ID_HANDLER(IDC_BTN_SELECT_BACKUP_PATH, OnSelectBackupPath)
        COMMAND_ID_HANDLER(IDC_BTN_INSTALL_IMEXT, OnInstallImext)
        COMMAND_HANDLER(IDC_EDIT_RWR_INSTALL_PATH, EN_KILLFOCUS, OnEditKillFocus)
        COMMAND_HANDLER(IDC_EDIT_BACKUP_PATH, EN_KILLFOCUS, OnEditKillFocus)
        REFLECT_NOTIFICATIONS() // 需要添加用于避免按钮底层出现灰色色块
    END_MSG_MAP()

    MainWindow();
    ~MainWindow();

private:
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled);
    LRESULT OnSize(UINT, WPARAM, LPARAM lParam, BOOL&);
    LRESULT OnContextMenu(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    void UpdateLayout(int clientWidth, int clientHeight);
    void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);
    void OnTimer(UINT_PTR nIDEvent);
    void OnLButtonDown(UINT nFlags, CPoint point);
    void UpdateAllStaticControls();
    void GetMinWinSize(LONG& minWinWidth, LONG& minWinHeight);
    void AdjustWndSize(int expectWidth = 0, int expectHeight = 0);
    LRESULT OnCtlColorStatic(UINT, WPARAM wParam, LPARAM lParam, BOOL&);
    
    void CreateToolBar(POINT pt);
    void SetInstallStatusText(const CString& text, COLORREF color = MY_COLOR_DEFAULT);
    void SetProgressText(const CString& text, COLORREF color = MY_COLOR_DEFAULT);
    void UpdateMD5CheckProgress(int progressNum, int totalNum);
    LRESULT OnUpdateTreeMain(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnSelectRwrInstallPath(WORD, WORD, HWND, BOOL&);
    LRESULT OnSelectBackupPath(WORD, WORD, HWND, BOOL&);
    LRESULT OnEditKillFocus(WORD, WORD, HWND hWndCtl, BOOL&);
    void UpdateRwrInstallPath(const CString& rwrPath);
    LRESULT OnInstallImext(WORD, WORD, HWND, BOOL&);

    CColorButton m_btnSelectRwrInstallPath;
    CMyEdit m_editRwrInstallPath;
    CColorButton m_btnSelectBackupPath;
    CMyEdit m_editBackupPath;
    CColorButton m_btnInstallIMExt;
    CStatic m_labelIMExtInstallStatus;
    CStatic m_labelProgress;
    CToolTipCtrl m_toolTipOnBtnInstallIMExt;

    int m_fontSize = 9;
    CFont m_font;
    LOGFONT m_lf;
    COLORREF m_labelIMExtInstallStatusColor = MY_COLOR_ERROR; // 默认红色
    CString m_labelIMExtInstallStatusText = _TR(IDS_NOT_RWR_PATH);
    COLORREF m_labelProgressColor = MY_COLOR_DEFAULT;
    CString m_labelProgressText = L"";
    CString m_toolTipOnBtnInstallIMExtText = _TR(IDS_TOOLTIP_ON_BTN_INSTALL_IMEXT);

    CComAutoCriticalSection m_csMD5CheckProgress;
    volatile LONG m_nMaxMD5CheckProgress = -1;
    volatile PVOID m_pPendingIMExtInstallStatusText = NULL;
    volatile PVOID m_pPendingProgressText = NULL;

    int m_btnTextHorizontalPadding = 0;
    int m_btnTextVerticalPadding = 0;
    int m_gridHorizontalSpacing = 0;
    int m_gridVerticalSpacing = 0;
    SIZE m_sizeBtnReferText = {0, 0};
    SIZE m_sizeEditReferText = {0, 0};
    int m_btnWidth = 0;
    int m_btnHeight = 0;
    int m_editWidth = 0;
    int m_minTreeHeight = 0;
    int m_minClientWidth = 0;
    int m_minClientHeight = 0;
    bool m_bShowTreeMain = false;

    CString m_rwrInstallPath = FileUtils::GetRealPath(FileUtils::GetExeDirectoryFile(L"..\\..\\..\\..\\common\\RunningWithRifles"));
    CString m_imextPath = FileUtils::GetExeDirectoryFile(L"RunningWithRifles");
    CString m_backupPath = FileUtils::GetRealPath(L".\\backup"); // 是否应该使用exe路径
    CString m_rwrConfigPath = FileUtils::NormalizePath(FileUtils::GetRoamingPath() + L"\\Running with rifles\\config.xml"); // 不需要判断是否存在

    bool m_installStatus = false;
    CString m_installedVer = L"";
    int m_installedVerCompareRet = 0;
    LONGLONG m_installTimestamp = 0LL;

    ImextVerifier m_imextVerifier;
    RWRConfigManager m_rwrConfigManager;

    int m_bGameCompatibility = ImextVerifier::GameCompatibility::UnknownExe;

    HANDLE m_hCheckInstallThread = NULL;
    void StartCheckImextInstallStatus();
    static unsigned __stdcall CheckImextInstallStatusThreadEntry(void* pParam) {
        if (!pParam) return 0;

        // 强制转换为类指针
        MainWindow* pThis = static_cast<MainWindow*>(pParam);
        
        // 执行实际的成员函数
        pThis->CheckImextInstallStatus();

        return 0;
    }
    HANDLE m_hInstallImextThread = NULL;
    void StartInstallImext();
    static unsigned __stdcall InstallImextThreadEntry(void* pParam) {
        if (!pParam) return 0;

        // 强制转换为类指针
        MainWindow* pThis = static_cast<MainWindow*>(pParam);
        
        // 执行实际的成员函数
        pThis->InstallImext();

        return 0;
    }
    void CheckImextInstallStatus();
    void InstallImext();
    void AddCheckResultTreeNodeData(LPCTSTR resultName, const cJSON* const sortedFilePath, COLORREF color = RGB(0, 0, 0), LPCTSTR overwriteAdditionalText = nullptr);

    static void MD5CheckProgressCallback(int progressNum, int totalNum, void* pCaller);
    static void ResetMD5CheckProgressCallback(void* pCaller);
    static void CopyFilesProgressCallback(int progressNum, int totalNum, void* pCaller);
    static void CopyFilesErrorCallback(LPCTSTR error, void* pCaller);
};