#include "ui/font_resize_tool_dialog.h"

LRESULT FontResizeToolDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    // 执行数据交换，将原生控件子类化为 CSnapTrackBar
    DoDataExchange(false);   // false 表示从控件到成员变量（此时子类化发生）

    // 开启双缓冲否则刷新文字会闪烁
    ModifyStyleEx(0, WS_EX_COMPOSITED);

    m_slider.Initialize(m_minVal, m_maxVal, m_snapStep, m_tickStep);
    
    CString unitValStr;
    unitValStr.Format(_T("%d%s"), m_minVal, m_unit);
    m_slider.AddLabel(m_minVal, unitValStr);
    unitValStr.Format(_T("%d%s"), (m_maxVal - m_minVal) / 2, m_unit);
    m_slider.AddLabel((m_maxVal - m_minVal) / 2, unitValStr);
    unitValStr.Format(_T("%d%s"), m_maxVal, m_unit);
    m_slider.AddLabel(m_maxVal, unitValStr);
    // 初始化初始值
    m_slider.SetPos(m_maxVal);
    unitValStr.Format(_T("%d"), m_maxVal);
    m_edit.SetWindowText(unitValStr);

    // 绑定控件句柄
    m_descLabel = (CStatic)GetDlgItem(IDC_RWR_FONT_RESIZE_TOOL_DESC_LABEL);
    m_pngLabel = (CStatic)GetDlgItem(IDC_RWR_FONT_RESIZE_TOOL_PNG_LABEL);
    m_resizePngLabel = (CStatic)GetDlgItem(IDC_RWR_FONT_RESIZE_TOOL_RESIZE_PNG_LABEL);
    m_editLabel = (CStatic)GetDlgItem(IDC_RWR_FONT_RESIZE_TOOL_EDIT_LABEL);
    m_btnOk = (CButton)GetDlgItem(IDOK_RWR_FONT_RESIZE_TOOL);
    m_btnCancel = (CButton)GetDlgItem(IDCANCEL_RWR_FONT_RESIZE_TOOL);

    // 设置文字
    m_descLabel.SetWindowText(m_descLabelText);
    m_editLabel.SetWindowText(m_unit);
    m_pngLabel.SetWindowText(m_pngLabelText);
    m_resizePngLabel.SetWindowText(m_resizePngLabelText);
    m_btnOk.SetWindowText(_TR(IDS_OK_BTN));
    m_btnCancel.SetWindowText(_TR(IDS_CANCEL_BTN));
    SetWindowText(_TR(IDS_RWR_FONT_RESIZE_TOOL));

    HICON hTitileIcon = (HICON)::LoadImage(ModuleHelper::GetResourceInstance(),
        MAKEINTRESOURCE(IDI_ICON_MAIN), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);

    SetIcon(hTitileIcon, FALSE); // 设置小图标（系统会自动将 hIcon 适配到小尺寸）

    // 一开始禁用提交按钮
    m_btnOk.EnableWindow(FALSE);

    InitSizeParam();

    if (m_fontResizeTool.ScanFonts()) {
        m_btnOk.EnableWindow(TRUE);
        // 内部已经自动AdjustWndSize
        UpdatePngLabelText(
            m_fontResizeTool.m_fontPngFileNameList,
            m_fontResizeTool.m_fontPngOldResolutionList,
            m_fontResizeTool.m_fontPngNewResolutionList,
            m_fontResizeTool.m_fontPngResizeProgressStatus
        );
    } else {
        m_pngLabelColor = MY_COLOR_ERROR;
        AdjustWndSize();
    }

    return TRUE;
}

LRESULT FontResizeToolDialog::OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if (m_hResizeFontsThread != NULL) {
        // 如果线程还在跑，无限期等待直到它结束
        WaitForSingleObject(m_hResizeFontsThread, INFINITE);
        CloseHandle(m_hResizeFontsThread);
        m_hResizeFontsThread = NULL;
    }

    EndDialog(wID);
    return 0;
}

void FontResizeToolDialog::InitSizeParam() {
    m_gridHorizontalSpacing = ScalePixelForWindow(m_hWnd, GRID_HORIZONTAL_SPACING);
    m_gridVerticalSpacing = ScalePixelForWindow(m_hWnd, GRID_VERTICAL_SPACING);

    CDCHandle dc = GetDC();
    
    HFONT hFont = GetFont();
    HFONT oldFont = (HFONT)SelectObject(dc, hFont);
    
    RECT rcText = { 0, 0, 0, 0 };

    // 参考文字宽度
    dc.DrawText(m_minClientWidthReferText, -1, &rcText, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    int referTextWidth = rcText.right - rcText.left;
    
    // 滑条, 长度自动, 宽度直接用GetMinHeight获取
    // 编辑框, 长宽由最大数值宽高决定
    m_minSliderWidth = ScalePixelForWindow(m_hWnd, TRACKBAR_WND_MIN_WIDTH);

    rcText = { 0, 0, 0, 0 };
    CString maxValStr;
    maxValStr.Format(_T("%d"), m_maxVal);
    dc.DrawText(maxValStr, -1, &rcText, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    UINT editMarginsLeft = 0;
    UINT editMarginsRight = 0;
    m_edit.GetMargins(editMarginsLeft, editMarginsRight);

    CRect rcEditWindow, rcEditClient;
    m_edit.GetWindowRect(rcEditWindow);
    m_edit.GetClientRect(rcEditClient);
    int ncWidth = rcEditWindow.Width() - rcEditClient.Width();
    int ncHeight = rcEditWindow.Height() - rcEditClient.Height();
    int caretPadding = GetSystemMetrics(SM_CXBORDER);

    m_editSize = {rcText.right - rcText.left + (int)editMarginsLeft + (int)editMarginsRight + ncWidth + caretPadding, rcText.bottom - rcText.top + ncHeight};

    rcText = { 0, 0, 0, 0 };
    dc.DrawText(m_unit, -1, &rcText, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    m_editUnitSize = {rcText.right - rcText.left, rcText.bottom - rcText.top};

    // 计算不包含左右两边留空的滑条这一行最小宽度
    int minSliderLineWith = m_minSliderWidth + m_gridHorizontalSpacing + m_editSize.cx + m_editUnitSize.cx;

    // PNG文件列表默认文字尺寸
    rcText = { 0, 0, 0, 0 };
    dc.DrawText(m_pngLabelText, -1, &rcText, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    SIZE minPngLabelTextSize = {rcText.right - rcText.left, rcText.bottom - rcText.top};

    // 按钮尺寸
    RECT rcButton;
    m_btnOk.GetClientRect(&rcButton);
    m_sizeButton = {rcButton.right - rcButton.left, rcButton.bottom - rcButton.top};
    m_btnSpacing = 2 * m_gridHorizontalSpacing;

    // 计算不包含左右两边留空的按钮这一行最小宽度
    int minButtonLineWidth = 2 * m_sizeButton.cx + m_btnSpacing;

    // 根据最大的最小宽度设置描述文字尺寸
    int minDescLabelTextWidth = referTextWidth > minSliderLineWith ? referTextWidth : minSliderLineWith;
    minDescLabelTextWidth = minDescLabelTextWidth > minPngLabelTextSize.cx ? minDescLabelTextWidth : minPngLabelTextSize.cx;
    minDescLabelTextWidth = minDescLabelTextWidth > minButtonLineWidth ? minDescLabelTextWidth : minButtonLineWidth;

    m_descLabelTextSize = {minDescLabelTextWidth, CalculateStaticHeight(m_descLabel, minDescLabelTextWidth, m_descLabelText)};
    
    SelectObject(dc, oldFont);

    // 窗口最小尺寸, 当获取到实际png文件列表以后最好再更新一次
    m_minClientSize.cx = 2 * m_gridHorizontalSpacing + minDescLabelTextWidth;
    m_minClientSize.cy = 
        5 * m_gridVerticalSpacing +
        m_descLabelTextSize.cy +
        m_slider.GetMinHeight() +
        minPngLabelTextSize.cy +
        m_sizeButton.cy;
}

void FontResizeToolDialog::UpdateLayout(int clientWidth, int clientHeight) {
    int descLabelWidth = clientWidth - 2 * m_gridHorizontalSpacing;
    int descLabelHeight = CalculateStaticHeight(m_descLabel, descLabelWidth, m_descLabelText);
    int sliderWidth = clientWidth - 3 * m_gridHorizontalSpacing - m_editSize.cx - m_editUnitSize.cx;
    int sliderHeight = m_slider.GetMinHeight();
    int pngLabelWidth = descLabelWidth;
    int pngLabelHeight = CalculateStaticHeight(m_pngLabel, pngLabelWidth, m_pngLabelText);
    int sliderLineHeight;
    // 按钮居中
    int buttonLineStartX = (clientWidth - 2 * m_gridHorizontalSpacing - m_btnSpacing - 2 * m_sizeButton.cx) / 2 + m_gridHorizontalSpacing;
    
    m_descLabel.MoveWindow(
        m_gridHorizontalSpacing,
        m_gridVerticalSpacing,
        descLabelWidth,
        descLabelHeight
    );
    m_descLabel.ShowWindow(SW_SHOW);

    if (sliderHeight > m_editSize.cy) {
        // 滑条比编辑框高, 编辑框上下居中
        sliderLineHeight = sliderHeight;
        m_slider.MoveWindow(
            m_gridHorizontalSpacing,
            2 * m_gridVerticalSpacing + descLabelHeight,
            sliderWidth,
            sliderHeight
        );
        m_edit.MoveWindow(
            2 * m_gridHorizontalSpacing + sliderWidth,
            2 * m_gridVerticalSpacing + descLabelHeight + (sliderHeight - m_editSize.cy) / 2,
            m_editSize.cx,
            m_editSize.cy
        );
        m_editLabel.MoveWindow(
            2 * m_gridHorizontalSpacing + sliderWidth + m_editSize.cx,
            2 * m_gridVerticalSpacing + descLabelHeight + (sliderHeight - m_editSize.cy) / 2,
            m_editUnitSize.cx,
            m_editUnitSize.cy
        );
    } else {
        // 滑条比编辑框低, 滑条上下居中
        sliderLineHeight = m_editSize.cy;
        m_slider.MoveWindow(
            m_gridHorizontalSpacing,
            2 * m_gridVerticalSpacing + descLabelHeight + (m_editSize.cy - sliderHeight) / 2,
            sliderWidth,
            sliderHeight
        );
        m_edit.MoveWindow(
            2 * m_gridHorizontalSpacing + sliderWidth,
            2 * m_gridVerticalSpacing + descLabelHeight,
            m_editSize.cx,
            m_editSize.cy
        );
        m_editLabel.MoveWindow(
            2 * m_gridHorizontalSpacing + sliderWidth + m_editSize.cx,
            2 * m_gridVerticalSpacing + descLabelHeight,
            m_editUnitSize.cx,
            m_editUnitSize.cy
        );
    }
    m_slider.ShowWindow(SW_SHOW);
    m_edit.ShowWindow(SW_SHOW);
    m_editLabel.ShowWindow(SW_SHOW);

    m_pngLabel.MoveWindow(
        m_gridHorizontalSpacing,
        3 * m_gridVerticalSpacing + descLabelHeight + sliderLineHeight,
        pngLabelWidth,
        pngLabelHeight
    );
    m_pngLabel.ShowWindow(SW_SHOW);
    m_resizePngLabel.MoveWindow(
        m_gridHorizontalSpacing,
        3 * m_gridVerticalSpacing + descLabelHeight + sliderLineHeight,
        pngLabelWidth,
        pngLabelHeight
    );
    m_resizePngLabel.ShowWindow(SW_SHOW);
    

    m_btnOk.MoveWindow(
        buttonLineStartX,
        clientHeight - m_gridVerticalSpacing - m_sizeButton.cy,
        m_sizeButton.cx,
        m_sizeButton.cy
    );
    m_btnOk.ShowWindow(SW_SHOW);
    m_btnCancel.MoveWindow(
        buttonLineStartX + m_sizeButton.cx + m_btnSpacing,
        clientHeight - m_gridVerticalSpacing - m_sizeButton.cy,
        m_sizeButton.cx,
        m_sizeButton.cy
    );
    m_btnCancel.ShowWindow(SW_SHOW);
}

void FontResizeToolDialog::AdjustWndSize(int expectWidth, int expectHeight) {
    int finalWidth;
    int finalHeight;
    LONG minWidth;
    LONG minHeight;
    GetMinWinSize(minWidth, minHeight);
    // 强制调整到最小尺寸
    if (expectWidth > 0) {
        finalWidth = (expectWidth < minWidth) ? minWidth : expectWidth;
    } else {
        finalWidth = minWidth;
    }
    if (expectHeight > 0) {
        finalHeight = (expectHeight < minHeight) ? minHeight : expectHeight;
    } else {
        finalHeight = minHeight;
    }
    // SWP_FRAMECHANGED 会触发 WM_NCCALCSIZE, 进而让系统查询 WM_GETMINMAXINFO
    SetWindowPos(NULL, 0, 0, finalWidth, finalHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

    // 更新整体布局
    RECT rcClient;
    GetClientRect(&rcClient);
    UpdateLayout(rcClient.right, rcClient.bottom);
}

void FontResizeToolDialog::GetMinWinSize(LONG& minWinWidth, LONG& minWinHeight) {
    // 1. 定义客户区最小尺寸
    RECT rcMin = { 0, 0, m_minClientSize.cx, m_minClientSize.cy }; 

    // 2. 根据窗口样式获取尺寸
    DWORD dwStyle = GetWindowLong(GWL_STYLE);
    DWORD dwExStyle = GetWindowLong(GWL_EXSTYLE);
    BOOL bMenu = (GetMenu() != NULL);
    AdjustWindowRectEx(&rcMin, dwStyle, bMenu, dwExStyle);

    // 3. 输出最小的窗口尺寸
    minWinWidth = rcMin.right - rcMin.left;
    minWinHeight = rcMin.bottom - rcMin.top;
}

int FontResizeToolDialog::CalculateStaticHeight(HWND staticText, int nWidth, LPCTSTR lpText) {
    CClientDC dc(staticText); // 获取设备上下文
    
    // 关键：必须获取并设置控件当前的字体，否则计算高度会不准
    HFONT hFont = (HFONT)::SendMessage(staticText, WM_GETFONT, 0, 0);
    HFONT hOldFont = dc.SelectFont(hFont);

    RECT rc = { 0, 0, nWidth, 0 }; // 宽度固定，高度设为0
    
    // DT_CALCRECT: 不实际绘制，只计算矩形大小
    // DT_WORDBREAK: 允许换行
    // DT_EDITCONTROL: 模拟编辑框行为（处理换行符更准）
    dc.DrawText(lpText, -1, &rc, DT_CALCRECT | DT_WORDBREAK);

    dc.SelectFont(hOldFont);
    
    return rc.bottom - rc.top;
}

int FontResizeToolDialog::CalculateStaticSingleLineWidth(HWND staticText, LPCTSTR lpText) {
    CClientDC dc(staticText); // 获取设备上下文
    
    // 关键：必须获取并设置控件当前的字体，否则计算高度会不准
    HFONT hFont = (HFONT)::SendMessage(staticText, WM_GETFONT, 0, 0);
    HFONT hOldFont = dc.SelectFont(hFont);

    RECT rc = { 0, 0, 0, 0 };

    dc.DrawText(lpText, -1, &rc, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);

    dc.SelectFont(hOldFont);
    
    return rc.right - rc.left;
}

LRESULT FontResizeToolDialog::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    HDC hdc = (HDC)wParam;
    HWND hwndLabel = (HWND)lParam;

    if (hwndLabel == m_pngLabel.m_hWnd) {
        SetTextColor(hdc, m_pngLabelColor);
        SetBkMode(hdc, TRANSPARENT); // 透明背景
        return (LRESULT)GetStockObject(NULL_BRUSH); // 需要注意更新文字时重绘父窗口
    }

    if (hwndLabel == m_resizePngLabel.m_hWnd) {
        SetTextColor(hdc, m_resizePngLabelColor);
        SetBkMode(hdc, TRANSPARENT); // 透明背景
        return (LRESULT)GetStockObject(NULL_BRUSH); // 需要注意更新文字时重绘父窗口
    }
    
    return 0;
}

void FontResizeToolDialog::OnLButtonDown(UINT nFlags, CPoint point) {
    // 点击空白处将焦点设置给主窗口
    SetFocus(); 
    
    // 点击后仍能处理其他消息
    SetMsgHandled(FALSE); 
}

LRESULT FontResizeToolDialog::OnReflectHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    LRESULT ret = 0;
    if ((HWND)lParam == m_slider.m_hWnd) {
        ret = SendMessage(m_slider.m_hWnd, OCM_USER_HSCROLL, wParam, lParam);
    }

    if (ret != 0) {
        bHandled = TRUE;
        return ret;
    } else {
        bHandled = FALSE;
        return 0;
    }
}

LRESULT FontResizeToolDialog::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if ((HWND)lParam == m_slider.m_hWnd) {
        // 1. 获取滑条当前的真实位置
        int curPos = m_slider.GetPos();

        // 5. 联动：将吸附后的准确数值同步显示到编辑框中
        // 注意：为了防止双向联动引发死循环，这里需要做特殊处理（见下文解析）
        CString strVal;
        strVal.Format(_T("%d"), curPos);
        
        // 获取编辑框当前文字，只有当文字确实不同时才更新，避免死循环
        CString strCurrent;
        m_edit.GetWindowText(strCurrent);
        if (strCurrent != strVal) {
            m_edit.SetWindowText(strVal);
        }

        UpdateTargetResolutionPercentage(curPos);
    }
    return 0;
}

LRESULT FontResizeToolDialog::OnEditKillFocus(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    // 1. 读取编辑框内的文本
    CString strText;
    m_edit.GetWindowText(strText);
    
    if (strText.IsEmpty()) {
        CString strVal;
        strVal.Format(_T("%d"), m_slider.GetPos());
        m_edit.SetWindowText(strVal);
        return 0;
    }

    // 2. 转换为整数
    int val = _ttoi(strText);

    // 3. 限制安全边界
    if (val < 0) {
        val = 0;
    }
    if (val > 100) {
        val = 100;
    }

    CString strVal;
    strVal.Format(_T("%d"), val);
    m_edit.SetWindowText(strVal);

    // 5. 反向联动：更新滑条位置
    // 同样，只有在位置真正改变时才调用 SetPos，防止死循环
    if (m_slider.GetPos() != val) {
        m_slider.SetPos(val);
    }

    UpdateTargetResolutionPercentage(val);

    return 0;
}

void FontResizeToolDialog::UpdateTargetResolutionPercentage(int silderPos) {
    if (silderPos > 0) {
        m_fontResizeTool.SetTargetResolutionPercentage(silderPos);
        UpdatePngLabelText(
            m_fontResizeTool.m_fontPngFileNameList,
            m_fontResizeTool.m_fontPngOldResolutionList,
            m_fontResizeTool.m_fontPngNewResolutionList,
            m_fontResizeTool.m_fontPngResizeProgressStatus
        );
        m_btnOk.EnableWindow(TRUE);
    } else {
        m_btnOk.EnableWindow(FALSE);
    }
}

LRESULT FontResizeToolDialog::OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    StartResizeFonts();
    return 0;
}

void FontResizeToolDialog::StartResizeFonts()
{
    if (m_hResizeFontsThread != NULL) {
        // 如果线程还在跑，无限期等待直到它结束
        WaitForSingleObject(m_hResizeFontsThread, INFINITE);
        CloseHandle(m_hResizeFontsThread);
        m_hResizeFontsThread = NULL;
    }

    // 禁用所有按钮和可动控件
    m_btnCancel.EnableWindow(FALSE);
    m_btnOk.EnableWindow(FALSE);
    m_slider.EnableWindow(FALSE);
    m_edit.EnableWindow(FALSE);

    m_hResizeFontsThread = (HANDLE)_beginthreadex(NULL, 0, &ResizeFontsThreadEntry, this, 0, NULL);
}

unsigned __stdcall FontResizeToolDialog::ResizeFontsThreadEntry(void* pParam) {
    if (!pParam) return 0;

    // 强制转换为类指针
    FontResizeToolDialog* pThis = static_cast<FontResizeToolDialog*>(pParam);
    
    // 执行实际的成员函数
    pThis->m_fontResizeTool.ResizeFonts(ResizeFontsCallback, pThis);

    // 启用控件
    pThis->m_btnCancel.EnableWindow(TRUE);
    pThis->m_btnOk.EnableWindow(TRUE);
    pThis->m_slider.EnableWindow(TRUE);
    pThis->m_edit.EnableWindow(TRUE);

    return 0;
}

void FontResizeToolDialog::ResizeFontsCallback(int progressNum, int totalNum, int ret, void* pCaller) {
    if (!pCaller) return;

    FontResizeToolDialog* pThis = static_cast<FontResizeToolDialog*>(pCaller);

    pThis->UpdatePngLabelText(
        pThis->m_fontResizeTool.m_fontPngFileNameList,
        pThis->m_fontResizeTool.m_fontPngOldResolutionList,
        pThis->m_fontResizeTool.m_fontPngNewResolutionList,
        pThis->m_fontResizeTool.m_fontPngResizeProgressStatus
    );
}

void FontResizeToolDialog::UpdatePngLabelText(
    const CSimpleArray<CString>& pngFileName,
    const CSimpleArray<CSize>& oldResolution,
    const CSimpleArray<CSize>& newResolution,
    const CSimpleArray<int>& resizeProgressStatus
) {
    if ((pngFileName.GetSize() != oldResolution.GetSize()) ||
        (pngFileName.GetSize() != newResolution.GetSize()) ||
        (pngFileName.GetSize() != resizeProgressStatus.GetSize())
    ) {
        return;
    }
    m_pngLabelText = _T("");
    m_resizePngLabelText = _T("");

    CString longestLine = _T("");
    int count = pngFileName.GetSize();
    for (int i = 0; i < count; i++) {
        CString newLine;
        if (oldResolution[i] == newResolution[i]) {
            newLine.Format(_T("%s (%dx%d)\n"), pngFileName[i], oldResolution[i].cx, oldResolution[i].cy);
            m_pngLabelText += newLine;
            m_resizePngLabelText += _T("\n");
        } else {
            newLine.Format(_T("%s (%dx%d) => (%dx%d)%s\n"),
                pngFileName[i], oldResolution[i].cx, oldResolution[i].cy,
                newResolution[i].cx, newResolution[i].cy,
                GetResizeProgressStatusText(resizeProgressStatus[i])
            );
            m_pngLabelText += _T("\n");
            m_resizePngLabelText += newLine;
        }

        if (newLine.GetLength() > longestLine.GetLength()) {
            longestLine = newLine;
        }
    }

    if (m_pngLabelText[m_pngLabelText.GetLength() - 1] == _T('\n')) {
        m_pngLabelText.Truncate(m_pngLabelText.GetLength() - 1);
    }
    if (m_resizePngLabelText[m_resizePngLabelText.GetLength() - 1] == _T('\n')) {
        m_resizePngLabelText.Truncate(m_resizePngLabelText.GetLength() - 1);
    }
    if (longestLine[longestLine.GetLength() - 1] == _T('\n')) {
        longestLine.Truncate(longestLine.GetLength() - 1);
    }

    int minPngLabelTextWidth = CalculateStaticSingleLineWidth(m_pngLabel, longestLine);
    if ((minPngLabelTextWidth + 2 * m_gridHorizontalSpacing) > m_minClientSize.cx) {
        m_minClientSize.cx = minPngLabelTextWidth + 2 * m_gridHorizontalSpacing;
    }

    int minPngLabelTextHeight = CalculateStaticHeight(m_pngLabel, m_minClientSize.cx - 2 * m_gridHorizontalSpacing, m_pngLabelText);
    
    m_minClientSize.cy = 
        5 * m_gridVerticalSpacing +
        m_descLabelTextSize.cy +
        m_slider.GetMinHeight() +
        minPngLabelTextHeight +
        m_sizeButton.cy;
    
    m_pngLabel.SetWindowText(m_pngLabelText);
    m_resizePngLabel.SetWindowText(m_resizePngLabelText);
    
    
    AdjustWndSize();

    // 会有闪烁问题
    CRect rc;
    m_resizePngLabel.GetWindowRect(&rc);
    ScreenToClient(&rc);
    InvalidateRect(&rc);
}

CString FontResizeToolDialog::GetResizeProgressStatusText(int resizeProgressStatus) {
    switch (resizeProgressStatus)
    {
    case FontResizeTool::ResizeProgressStatus::Resizing:
        return _T(" ") + CString(_TR(IDS_RESIZE_PNG_PROGRESS_STATUS_RESIZING));
    case FontResizeTool::ResizeProgressStatus::Succeeded:
        return _T(" ") + CString(_TR(IDS_RESIZE_PNG_PROGRESS_STATUS_SUCCEEDED));
    case FontResizeTool::ResizeProgressStatus::Failed:
        return _T(" ") + CString(_TR(IDS_RESIZE_PNG_PROGRESS_STATUS_FAILED));
    case FontResizeTool::ResizeProgressStatus::Idle:
    default:
        return _T("");
    }
}