#include "ui/ask_dialog.h"

AskDialog::AskDialog(LPCTSTR title, LPCTSTR text, LPCTSTR sysIcon, bool askMode, const cJSON* const fileRelPathInfo, LPCTSTR rwrInstallPath, LPCTSTR imextPath) : 
FileTreeView<AskDialog>(IDC_TREE_ASK_DIALOG, WM_USER_UPDATE_TREE_DIALOG),
m_strTitle(title),
m_strText(text),
m_sysIcon(sysIcon),
m_askMode(askMode),
m_nSelection(IDCANCEL)
{
    if (cJSON_IsObject(fileRelPathInfo) && fileRelPathInfo->child) {
        m_bShowTreeDialog = true;
        CString strRwrInstallPath(rwrInstallPath);
        CString strImextPath(imextPath);
        FileTreeView::SetRwrInstallPath(strRwrInstallPath);
        FileTreeView::SetImextPath(strImextPath);

        // 初始化树节点信息
        InitTreeNodeDataRoot();
        FileTreeView::insertFileRelPathNode(&m_treeNodeDataRoot, fileRelPathInfo, MY_COLOR_DEFAULT, _T(""), 3);
    }
}

AskDialog::AskDialog(LPCTSTR title, LPCTSTR text, LPCTSTR sysIcon, bool askMode, const CSimpleArray<CString>* const fileRelPathList, LPCTSTR rwrInstallPath, LPCTSTR imextPath) : 
FileTreeView<AskDialog>(IDC_TREE_ASK_DIALOG, WM_USER_UPDATE_TREE_DIALOG),
m_strTitle(title),
m_strText(text),
m_sysIcon(sysIcon),
m_askMode(askMode),
m_nSelection(IDCANCEL)
{
    if (fileRelPathList && fileRelPathList->GetSize() > 0) {
        m_bShowTreeDialog = true;
        CString strRwrInstallPath(rwrInstallPath);
        CString strImextPath(imextPath);
        FileTreeView::SetRwrInstallPath(strRwrInstallPath);
        FileTreeView::SetImextPath(strImextPath);

        // 初始化树节点信息
        InitTreeNodeDataRoot();
        FileTreeView::insertFileRelPathNode(&m_treeNodeDataRoot, fileRelPathList, MY_COLOR_DEFAULT, _T(""), 3);
    }
}

LRESULT AskDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    InitSizeParam();

    CenterWindow(GetParent());
    // 设置按钮文字翻译
    SetDlgItemText(IDOK_ASK_DIALOG, _TR(IDS_OK_BTN));
    SetDlgItemText(IDCANCEL_ASK_DIALOG, _TR(IDS_CANCEL_BTN));
    // 设置标题
    SetWindowText(m_strTitle);
    HICON hTitileIcon = (HICON)::LoadImage(ModuleHelper::GetResourceInstance(),
        MAKEINTRESOURCE(IDI_ICON_MAIN), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);

    SetIcon(hTitileIcon, TRUE);  // 设置大图标
    SetIcon(hTitileIcon, FALSE); // 设置小图标（系统会自动将 hIcon 适配到小尺寸）
    // 设置描述文本
    SetDlgItemText(IDC_STATIC_TEXT_ASK_DIALOG, m_strText);

    // 加载 Windows 默认提示图标 (如 IDI_QUESTION 或 IDI_INFORMATION)
    HICON hIcon = ::LoadIcon(NULL, m_sysIcon);
    if (hIcon) {
        CStatic wndIcon = (CStatic)GetDlgItem(IDC_ICON_HOLDER_ASK_DIALOG);
        wndIcon.SetIcon(hIcon);
    }

    AdjustWndSize();

    // 加载树节点
    if (m_bShowTreeDialog) SendMessageW(WM_USER_UPDATE_TREE_DIALOG);

    // 播放默认提示音
    ::MessageBeep(MB_ICONQUESTION);

    return TRUE;
}

LRESULT AskDialog::OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    m_nSelection = wID;
    EndDialog(m_nSelection);
    return 0;
}

LRESULT AskDialog::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    int w = LOWORD(lParam);
    int h = HIWORD(lParam);
    
    UpdateLayout(w, h);
    
    return 0;
}

/*void AskDialog::OnGetMinMaxInfo(LPMINMAXINFO lpMMI) {
    GetMinWinSize(lpMMI->ptMinTrackSize.x, lpMMI->ptMinTrackSize.y);
}*/

LRESULT AskDialog::OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    // 1. 将 lParam 强转为结构体指针
    LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;

    // 2. 设置最小尺寸 (像素)
    //lpMMI->ptMinTrackSize.x = 400; // 最小宽度
    //lpMMI->ptMinTrackSize.y = 300; // 最小高度
    GetMinWinSize(lpMMI->ptMinTrackSize.x, lpMMI->ptMinTrackSize.y);

    // 3. 设置最大尺寸 (可选)
    // lpMMI->ptMaxTrackSize.x = 800;

    // 4. 重要：如果你希望 CDialogResize 继续处理（例如它内部的缩放逻辑），
    // 应当将 bHandled 设为 FALSE；
    // 如果你希望自己完全接管，不让系统或基类再乱动，设为 TRUE。
    bHandled = TRUE; 
    
    return 0;
}

void AskDialog::InitSizeParam() {
    // 计算Text的宽度
    HWND staticText = (HWND)GetDlgItem(IDC_STATIC_TEXT_ASK_DIALOG);
    CClientDC dc(staticText); // 获取设备上下文
    HFONT hFont = (HFONT)::SendMessage(staticText, WM_GETFONT, 0, 0);
    HFONT hOldFont = dc.SelectFont(hFont);

    // 获取参考文字对应的尺寸
    RECT rcReferText = { 0, 0, 0, 0 }; 
    // DT_CALCRECT: 只计算不绘图
    // DT_SINGLELINE: 强制单行，忽略换行符
    // DT_NOPREFIX: 如果文字里有 '&'，不把它当成下划线快捷键计算
    dc.DrawText(m_strSizeReferText, -1, &rcReferText, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    m_sizeReferText.cx = rcReferText.right - rcReferText.left;
    m_sizeReferText.cy = rcReferText.bottom - rcReferText.top;

    // 根据参考文字尺寸计算显示字符串换行的尺寸
    RECT rcText = { 0, 0, m_sizeReferText.cx, 0 }; // 宽度固定，高度设为0
    // DT_CALCRECT: 不实际绘制，只计算矩形大小
    // DT_WORDBREAK: 允许换行
    // DT_EDITCONTROL: 模拟编辑框行为（处理换行符更准）
    dc.DrawText(m_strText, -1, &rcText, DT_CALCRECT | DT_WORDBREAK);
    m_sizeText.cx = rcText.right - rcText.left;
    m_sizeText.cy = rcText.bottom - rcText.top;

    dc.SelectFont(hOldFont);

    // 根据Text的单行高度计算Tree的最小高度
    m_minTreeHeight = 5 * m_sizeReferText.cy;

    // 获取ICON的尺寸
    CStatic staticIcon = (CStatic)GetDlgItem(IDC_ICON_HOLDER_ASK_DIALOG);
    RECT rcIcon;
    staticIcon.GetClientRect(&rcIcon);
    m_sizeIcon.cx = rcIcon.right - rcIcon.left;
    m_sizeIcon.cy = m_sizeIcon.cx;  // 强制图标长宽一致

    // 获取按钮尺寸
    CButton btnOk = (CButton)GetDlgItem(IDOK_ASK_DIALOG);
    RECT rcButton;
    btnOk.GetClientRect(&rcButton);
    m_sizeButton.cx = rcButton.right - rcButton.left;
    m_sizeButton.cy = rcButton.bottom - rcButton.top;

    // 根据DPI缩放计算间距
    m_gridHorizontalSpacing = ScalePixelForWindow(m_hWnd, GRID_HORIZONTAL_SPACING);
    m_gridVerticalSpacing = ScalePixelForWindow(m_hWnd, GRID_VERTICAL_SPACING);

    // 确定窗口最小尺寸(客户区)
    int minClientWidthA = 3 * m_gridHorizontalSpacing + m_sizeIcon.cx + m_sizeText.cx;
    m_btnSpacing = 2 * m_gridHorizontalSpacing;
    int minClientWidthB = 2 * m_gridHorizontalSpacing + m_btnSpacing + 2 * m_sizeButton.cx;
    m_minClientWidth = minClientWidthA > minClientWidthB ? minClientWidthA : minClientWidthB;
    m_minClientHeight = 3 * m_gridVerticalSpacing + m_sizeButton.cy + (m_sizeIcon.cy > m_sizeText.cy ? m_sizeIcon.cy : m_sizeText.cy);
}

void AskDialog::UpdateLayout(int clientWidth, int clientHeight) {
    CStatic staticIcon = (CStatic)GetDlgItem(IDC_ICON_HOLDER_ASK_DIALOG);
    CStatic staticText = (CStatic)GetDlgItem(IDC_STATIC_TEXT_ASK_DIALOG);
    CButton btnOk = (CButton)GetDlgItem(IDOK_ASK_DIALOG);
    CButton btnCancel = (CButton)GetDlgItem(IDCANCEL_ASK_DIALOG);

    int currentTextWidth = clientWidth - (3 * m_gridHorizontalSpacing + m_sizeIcon.cx);
    int currentTextHeight = CalculateStaticHeight(staticText, currentTextWidth, m_strText);
    int lineTextHeight = (m_sizeIcon.cy > currentTextHeight ? m_sizeIcon.cy : currentTextHeight);

    if (m_sizeIcon.cy > currentTextHeight) {
        // 图标比文字高, 文字上下居中
        staticIcon.MoveWindow(
            m_gridHorizontalSpacing,
            m_gridVerticalSpacing,
            m_sizeIcon.cx,
            m_sizeIcon.cy
        );
        staticText.MoveWindow(
            2 * m_gridHorizontalSpacing + m_sizeIcon.cx,
            m_gridVerticalSpacing + (m_sizeIcon.cy - currentTextHeight) / 2,
            currentTextWidth,
            currentTextHeight
        );
    } else {
        // 图标比文字低, 图标上下居中
        staticIcon.MoveWindow(
            m_gridHorizontalSpacing,
            m_gridVerticalSpacing + (currentTextHeight - m_sizeIcon.cy) / 2,
            m_sizeIcon.cx,
            m_sizeIcon.cy
        );
        staticText.MoveWindow(
            2 * m_gridHorizontalSpacing + m_sizeIcon.cx,
            m_gridVerticalSpacing,
            currentTextWidth,
            currentTextHeight
        );
    }
    staticIcon.ShowWindow(SW_SHOW);
    staticText.ShowWindow(SW_SHOW);
    
    if (m_bShowTreeDialog) {
        m_tree.MoveWindow(
            m_gridHorizontalSpacing,
            2 * m_gridVerticalSpacing + lineTextHeight,
            clientWidth - 2 * m_gridHorizontalSpacing,
            clientHeight - (4 * m_gridVerticalSpacing + lineTextHeight + m_sizeButton.cy)
        );
        m_tree.ShowWindow(SW_SHOW);
    } else {
        m_tree.ShowWindow(SW_HIDE);
    }

    // 两按钮居中 int lineButtonStartX = (clientWidth - 2 * m_gridHorizontalSpacing - m_btnSpacing - 2 * m_sizeButton.cx) / 2 + m_gridHorizontalSpacing;
    int lineButtonStartX;
    if (m_askMode) {
        // 两个按钮靠右
        lineButtonStartX = clientWidth - m_gridHorizontalSpacing - m_btnSpacing - 2 * m_sizeButton.cx;
    } else {
        // 只有一个按钮靠右
        lineButtonStartX = clientWidth - m_gridHorizontalSpacing - m_sizeButton.cx;
    }
    
    btnOk.MoveWindow(
        lineButtonStartX,
        clientHeight - m_gridVerticalSpacing - m_sizeButton.cy,
        m_sizeButton.cx,
        m_sizeButton.cy
    );
    btnOk.ShowWindow(SW_SHOW);

    if (m_askMode) {
        btnCancel.MoveWindow(
            lineButtonStartX + m_sizeButton.cx + m_btnSpacing,
            clientHeight - m_gridVerticalSpacing - m_sizeButton.cy,
            m_sizeButton.cx,
            m_sizeButton.cy
        );
        btnCancel.ShowWindow(SW_SHOW);
    } else {
        btnCancel.ShowWindow(SW_HIDE);
    }

    Invalidate();
}

void AskDialog::AdjustWndSize(int expectWidth, int expectHeight) {
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

void AskDialog::GetMinWinSize(LONG& minWinWidth, LONG& minWinHeight) {
    // 1. 定义客户区最小尺寸
    int minClientHeight = m_minClientHeight;
    if (m_bShowTreeDialog)
        minClientHeight += m_minTreeHeight + m_gridVerticalSpacing;
    RECT rcMin = { 0, 0, m_minClientWidth, minClientHeight }; 

    // 2. 根据窗口样式获取尺寸
    DWORD dwStyle = GetWindowLong(GWL_STYLE);
    DWORD dwExStyle = GetWindowLong(GWL_EXSTYLE);
    BOOL bMenu = (GetMenu() != NULL);
    AdjustWindowRectEx(&rcMin, dwStyle, bMenu, dwExStyle);

    // 3. 输出最小的窗口尺寸
    minWinWidth = rcMin.right - rcMin.left;
    minWinHeight = rcMin.bottom - rcMin.top;
}

int AskDialog::CalculateStaticHeight(HWND staticText, int nWidth, LPCTSTR lpText) {
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