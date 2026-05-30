#pragma once

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlddx.h>
#include <atlframe.h>
#include <atltypes.h>

#include "ui/common.h"
#include "ui/my_edit.h"
#include "ui/snap_track_bar.h"
#include "i18n/i18n.h"
#include "core/font_resize_tool.h"

class FontResizeToolDialog : public CDialogImpl<FontResizeToolDialog>, public CWinDataExchange<FontResizeToolDialog>
{
public:
    enum { IDD = IDD_RWR_FONT_RESIZE_TOOL };

    BEGIN_DDX_MAP(FontResizeToolDialog)
        DDX_CONTROL(IDC_RWR_FONT_RESIZE_TOOL_SLIDER, m_slider)   // 关键：将 IDC_TRACKBAR 与 m_trackBar 关联
        DDX_CONTROL(IDC_RWR_FONT_RESIZE_TOOL_EDIT, m_edit)
    END_DDX_MAP()

    BEGIN_MSG_MAP(FontResizeToolDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        // 1. 监听滑条的滚动消息（水平滑条用 WM_HSCROLL，垂直用 WM_VSCROLL）
        MESSAGE_HANDLER(WM_HSCROLL, OnReflectHScroll)
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
        MSG_WM_LBUTTONDOWN(OnLButtonDown)
        // 2. 监听编辑框内容改变的通知
        COMMAND_HANDLER(IDC_RWR_FONT_RESIZE_TOOL_EDIT, EN_KILLFOCUS, OnEditKillFocus)
        COMMAND_ID_HANDLER(IDCANCEL, OnClose)
        COMMAND_ID_HANDLER(IDCANCEL_RWR_FONT_RESIZE_TOOL, OnClose)
        COMMAND_ID_HANDLER(IDOK_RWR_FONT_RESIZE_TOOL, OnOk)
    END_MSG_MAP()

    FontResizeToolDialog(const CString& rwrPath) : m_rwrPath(rwrPath), m_fontResizeTool(rwrPath) {}

private:
    // 声明 WTL 控件包装对象
    CSnapTrackBar m_slider;
    CMyEdit       m_edit;
    CStatic       m_editLabel;
    CStatic       m_descLabel;
    CStatic       m_pngLabel;
    CStatic       m_resizePngLabel;
    CButton       m_btnOk;
    CButton       m_btnCancel;

    int m_minVal = 0;
    int m_maxVal = 100;
    int m_snapStep = 5;
    int m_tickStep = 25;
    CString m_unit = _T("%");
    CString m_descLabelText = _TR(IDS_RWR_FONT_RESIZE_TOOL_DESC) + CString(_T("\n")) + _TR(IDS_RWR_FONT_PROBLEM_DESC);
    CString m_pngLabelText = _TR(IDS_SEARCH_PNG_FAILED);
    CString m_resizePngLabelText = _T("");
    COLORREF m_pngLabelColor = MY_COLOR_DEFAULT;
    COLORREF m_resizePngLabelColor = MY_COLOR_CRITICAL;

    int m_gridHorizontalSpacing = 0;
    int m_gridVerticalSpacing = 0;
    int m_minSliderWidth = 150;
    int m_btnSpacing = 0;
    SIZE m_sizeButton = {0, 0};
    SIZE m_editSize = {0, 0};
    SIZE m_editUnitSize = {0, 0};
    SIZE m_descLabelTextSize = {0, 0};
    SIZE m_minClientSize = {500, 300};
    CString m_minClientWidthReferText = _T("chinese_input_font_outline_100.fontdef (7600x7600) => (6000x6000) Succeeded");

    CString m_rwrPath = _T("");
    FontResizeTool m_fontResizeTool;
    HANDLE m_hResizeFontsThread = NULL;

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(WORD, WORD wID, HWND, BOOL&);
    LRESULT OnOk(WORD, WORD wID, HWND, BOOL&);
    void OnLButtonDown(UINT nFlags, CPoint point);
    LRESULT OnReflectHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEditKillFocus(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCtlColorStatic(UINT, WPARAM wParam, LPARAM lParam, BOOL&);

    void InitSizeParam();
    void UpdateLayout(int clientWidth, int clientHeight);
    void AdjustWndSize(int expectWidth = 0, int expectHeight = 0);
    void GetMinWinSize(LONG& minWinWidth, LONG& minWinHeight);
    int CalculateStaticHeight(HWND staticText, int nWidth, LPCTSTR lpText);
    int CalculateStaticSingleLineWidth(HWND staticText, LPCTSTR lpText);

    void UpdateTargetResolutionPercentage(int silderPos);
    void UpdatePngLabelText(
        const CSimpleArray<CString>& pngFileName,
        const CSimpleArray<CSize>& oldResolution,
        const CSimpleArray<CSize>& newResolution,
        const CSimpleArray<int>& resizeProgressStatus
    );
    CString GetResizeProgressStatusText(int resizeProgressStatus);
    void StartResizeFonts();
    static unsigned __stdcall ResizeFontsThreadEntry(void* pParam);
    static void ResizeFontsCallback(int progressNum, int totalNum, int ret, void* pCaller);
};