#include "ui/main_window.h"

MainWindow::MainWindow(): FileTreeView<MainWindow>(IDC_TREE_MAIN, WM_USER_UPDATE_TREE_MAIN) {
    //需要初始化m_rwrInstallPath和m_imextPath
    if (!FileUtils::FileExists(m_imextPath)) {
        CString errorMsg;
        errorMsg.Format(L"IMExt file path \"%s\" not found!", m_imextPath);
        FastFatalError(errorMsg);
    }
    FileTreeView::SetRwrInstallPath(m_rwrInstallPath);
    FileTreeView::SetImextPath(m_imextPath);

    m_imextVerifier.Init(
        FileUtils::GetExeDirectoryFile(L"install_conf.json"),
        MD5CheckProgressCallback,
        ResetMD5CheckProgressCallback,
        this
    );
}

MainWindow::~MainWindow() {
    if (m_hCheckInstallThread != NULL) {
        WaitForSingleObject(m_hCheckInstallThread, INFINITE);
        CloseHandle(m_hCheckInstallThread);
        m_hCheckInstallThread = NULL;
    }
}

LRESULT MainWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    // 1. 字体名称
    memset(&m_lf, 0, sizeof(LOGFONT));
    _tcscpy_s(m_lf.lfFaceName, _T("Segoe UI"));

    // 2. 修改字号 (lfHeight)
    // 注意：Win32 中 lfHeight 为负数时表示点数 (Pixels)
    // 如果你想设置 12 号字，可以使用 MulDiv 计算，或者直接设为 -16 (约等于 12pt)
    CClientDC dc(m_hWnd);
    m_lf.lfHeight = -MulDiv(m_fontSize, GetDeviceCaps(dc, LOGPIXELSY), 72);

    m_lf.lfWeight = FW_MEDIUM;
    m_lf.lfCharSet = DEFAULT_CHARSET;
    m_lf.lfQuality = CLEARTYPE_QUALITY;

    // 4. 创建并保存新字体
    if (!m_font.IsNull()) m_font.DeleteObject();
    m_font.CreateFontIndirect(&m_lf);

    // 计算最大的按钮的文字尺寸
    // 1. 获取文字内容
    CString btnReferTextEN(I18n::stringEN[IDS_SELECT_BACKUP_PATH]);
    CString btnReferTextZH(I18n::stringZH[IDS_SELECT_BACKUP_PATH]);
    CString btnReferText = (btnReferTextEN.GetLength() > btnReferTextZH.GetLength() ? btnReferTextEN : btnReferTextZH);
    CString editReferText = m_rwrInstallPath.GetLength() > m_backupPath.GetLength() ? m_rwrInstallPath : m_backupPath;
    // 2. 获取 DC 并绑定到 CDCHandle
    
    // 3. 必须选中按钮当前字体
    HFONT hOldFont = dc.SelectFont(m_font);
    // 4. 使用 CDCHandle 的 GetTextExtent 计算
    dc.GetTextExtent(btnReferText, btnReferText.GetLength(), &m_sizeBtnReferText);
    dc.GetTextExtent(editReferText, editReferText.GetLength(), &m_sizeEditReferText);
    // 5. 恢复字体
    dc.SelectFont(hOldFont);

    m_btnTextHorizontalPadding = ScalePixelForWindow(m_hWnd, BTN_TEXT_HORIZONTAL_PADDING);
    m_btnTextVerticalPadding = ScalePixelForWindow(m_hWnd, BTN_TEXT_VERTICAL_PADDING);
    m_gridHorizontalSpacing = ScalePixelForWindow(m_hWnd, GRID_HORIZONTAL_SPACING);
    m_gridVerticalSpacing = ScalePixelForWindow(m_hWnd, GRID_VERTICAL_SPACING);

    m_btnWidth = 2 * m_btnTextHorizontalPadding + m_sizeBtnReferText.cx;
    m_btnHeight =  2 * m_btnTextVerticalPadding + m_sizeBtnReferText.cy;

    m_editWidth = m_btnTextHorizontalPadding + m_sizeEditReferText.cx;
    m_editWidth = (m_editWidth < 2 * m_btnWidth) ? 2 * m_btnWidth : m_editWidth;
    
    m_minTreeHeight = 5 * m_btnHeight;

    m_minClientWidth = 3 * m_gridHorizontalSpacing + m_btnWidth + m_editWidth;
    m_minClientHeight = 4 * m_gridVerticalSpacing + 3 * m_btnHeight;

    m_btnSelectRwrInstallPath.Create(m_hWnd, CButton::rcDefault, _TR(IDS_SELECT_RWR_PATH),
        WS_CHILD | WS_VISIBLE, 0, IDC_BTN_SELECT_RWR_INSTALL_PATH);
    m_btnSelectRwrInstallPath.SetFont(m_font);
    m_editRwrInstallPath.Create(m_hWnd, CEdit::rcDefault, m_rwrInstallPath,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 0, IDC_EDIT_RWR_INSTALL_PATH);
    m_editRwrInstallPath.SetFont(m_font);

    m_btnSelectBackupPath.Create(m_hWnd, CButton::rcDefault, _TR(IDS_SELECT_BACKUP_PATH),
        WS_CHILD | WS_VISIBLE, 0, IDC_BTN_SELECT_BACKUP_PATH);
    m_btnSelectBackupPath.SetFont(m_font);
    m_btnSelectBackupPath.EnableWindow(FALSE);
    m_editBackupPath.Create(m_hWnd, CEdit::rcDefault, m_backupPath,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 0, IDC_EDIT_BACKUP_PATH);
    m_editBackupPath.SetFont(m_font);

    m_btnInstallIMExt.Create(m_hWnd, CButton::rcDefault, _TR(IDS_INSTALL_IMEXT),
        WS_CHILD | WS_VISIBLE, 0, IDC_BTN_INSTALL_IMEXT);
    m_btnInstallIMExt.SetColors(RGB(60, 179, 113), RGB(255, 255, 255));
    m_btnInstallIMExt.SetFont(m_font);
    m_btnInstallIMExt.EnableWindow(FALSE);
    
    // 增加安装按钮tooltip
    m_toolTipOnBtnInstallIMExt.Create(m_hWnd, CToolTipCtrl::rcDefault, NULL, TTS_ALWAYSTIP);
    TOOLINFO ti = {0};
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS; 
    ti.hwnd = m_hWnd;
    ti.uId = (UINT_PTR)m_btnInstallIMExt.m_hWnd;
    ti.lpszText = (LPTSTR)m_toolTipOnBtnInstallIMExtText.GetString();
    m_toolTipOnBtnInstallIMExt.AddTool(&ti);
    m_toolTipOnBtnInstallIMExt.SetDelayTime(TTDT_INITIAL, 0);
    m_toolTipOnBtnInstallIMExt.SetDelayTime(TTDT_AUTOPOP, 10000);
    m_toolTipOnBtnInstallIMExt.SetDelayTime(TTDT_RESHOW, 300);
    m_toolTipOnBtnInstallIMExt.Activate(TRUE);

    m_labelIMExtInstallStatus.Create(m_hWnd, CStatic::rcDefault, m_labelIMExtInstallStatusText,
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 0, IDC_LABEL_IMEXT_INSTALL_STATUS);
    m_labelIMExtInstallStatus.SetFont(m_font);

    FileTreeView::m_tree.SetFont(m_font);

    m_labelProgress.Create(m_hWnd, CStatic::rcDefault, m_labelProgressText,
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 0, IDC_LABEL_IMEXT_INSTALL_STATUS);
    m_labelProgress.SetFont(m_font);

    CString title;
    title.Format(L"%s - IMExt %s", _TR(IDS_TITLE_MAIN), m_imextVerifier.m_version);
    SetWindowText(title);

    HICON hIcon = (HICON)::LoadImage(ModuleHelper::GetResourceInstance(),
        MAKEINTRESOURCE(IDI_ICON_MAIN), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);

    SetIcon(hIcon, TRUE);  // 设置大图标
    SetIcon(hIcon, FALSE); // 设置小图标（系统会自动将 hIcon 适配到小尺寸）

    AdjustWndSize();

    SetTimer(IDT_UPDATE_STATIC_TIMER, 50);
    
    // 不需要判断系统版本禁用
    if (FileUtils::FileExists(m_rwrInstallPath + _T("\\") + ImextVerifier::strRwrGameExe)) {
        StartCheckImextInstallStatus();
    }

    return 0;
}

LRESULT MainWindow::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    KillTimer(IDT_UPDATE_STATIC_TIMER);
    LabelDrawInfo* p1 = (LabelDrawInfo*)InterlockedExchangePointer((void**)(&m_pPendingIMExtInstallStatusText), NULL);
    if (p1) delete p1;
    LabelDrawInfo* p2 = (LabelDrawInfo*)InterlockedExchangePointer((void**)(&m_pPendingProgressText), NULL);
    if (p2) delete p2;

    PostQuitMessage(0);
    bHandled = FALSE;
    return 0;
}

// ---- 布局 ----
LRESULT MainWindow::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    int w = LOWORD(lParam);
    int h = HIWORD(lParam);
    
    UpdateLayout(w, h);
    
    return 0;
}

void MainWindow::UpdateLayout(int clientWidth, int clientHeight)
{
    m_btnSelectRwrInstallPath.MoveWindow(
        m_gridHorizontalSpacing,
        m_gridVerticalSpacing,
        m_btnWidth,
        m_btnHeight
    );
    m_editRwrInstallPath.MoveWindow(
        2 * m_gridHorizontalSpacing + m_btnWidth,
        m_gridVerticalSpacing,
        clientWidth - (3 * m_gridHorizontalSpacing + m_btnWidth),
        m_btnHeight
    );

    m_btnSelectBackupPath.MoveWindow(
        m_gridHorizontalSpacing,
        2 * m_gridVerticalSpacing + m_btnHeight,
        m_btnWidth,
        m_btnHeight
    );
    m_editBackupPath.MoveWindow(
        2 * m_gridHorizontalSpacing + m_btnWidth,
        2 * m_gridVerticalSpacing + m_btnHeight,
        clientWidth - (3 * m_gridHorizontalSpacing + m_btnWidth),
        m_btnHeight
    );

    m_btnInstallIMExt.MoveWindow(
        m_gridHorizontalSpacing,
        3 * m_gridVerticalSpacing + 2 * m_btnHeight,
        m_btnWidth,
        m_btnHeight
    );
    m_labelIMExtInstallStatus.MoveWindow(
        2 * m_gridHorizontalSpacing + m_btnWidth,
        3 * m_gridVerticalSpacing + 2 * m_btnHeight,
        clientWidth - (3 * m_gridHorizontalSpacing + m_btnWidth),
        m_btnHeight
    );

    if (!m_labelProgressText.IsEmpty())
    {
        m_labelProgress.ShowWindow(SW_SHOW);
        if (m_bShowTreeMain)
        {
            m_tree.ShowWindow(SW_SHOW);
            FileTreeView::m_tree.MoveWindow(
                m_gridHorizontalSpacing,
                4 * m_gridVerticalSpacing + 3 * m_btnHeight,
                clientWidth - 2 * m_gridHorizontalSpacing,
                clientHeight - (6 * m_gridVerticalSpacing + 3 * m_btnHeight + m_sizeBtnReferText.cy)
            );
            m_labelProgress.MoveWindow(
                m_gridHorizontalSpacing,
                clientHeight - (m_gridVerticalSpacing + m_sizeBtnReferText.cy),
                clientWidth - 2 * m_gridHorizontalSpacing,
                m_sizeBtnReferText.cy // 使用字体高度避免最后一行空间太多
            );
        }
        else
        {
            m_tree.ShowWindow(SW_HIDE);
            // 没有树的情况下, 垂直空间用m_labelProgress填满
            m_labelProgress.MoveWindow(
                m_gridHorizontalSpacing,
                4 * m_gridVerticalSpacing + 3 * m_btnHeight,
                clientWidth - 2 * m_gridHorizontalSpacing,
                clientHeight - (5 * m_gridVerticalSpacing + 3 * m_btnHeight)
            );
        }
        
    }
    else
    {
        m_labelProgress.ShowWindow(SW_HIDE);
        if (m_bShowTreeMain)
        {
            m_tree.ShowWindow(SW_SHOW);
            FileTreeView::m_tree.MoveWindow(
                m_gridHorizontalSpacing,
                4 * m_gridVerticalSpacing + 3 * m_btnHeight,
                clientWidth - 2 * m_gridHorizontalSpacing,
                clientHeight - (5 * m_gridVerticalSpacing + 3 * m_btnHeight)
            );
        } else {
            m_tree.ShowWindow(SW_HIDE);
        }
    }

    Invalidate();
}

// 限制窗口最小尺寸
void MainWindow::OnGetMinMaxInfo(LPMINMAXINFO lpMMI) {
    GetMinWinSize(lpMMI->ptMinTrackSize.x, lpMMI->ptMinTrackSize.y);
}

void MainWindow::GetMinWinSize(LONG& minWinWidth, LONG& minWinHeight)
{
    // 1. 定义客户区最小尺寸
    int minClientHeight = m_minClientHeight;
    if (m_bShowTreeMain)
        minClientHeight += m_minTreeHeight;
    if (!m_labelProgressText.IsEmpty())
        minClientHeight += m_sizeBtnReferText.cy + m_gridVerticalSpacing;
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

void MainWindow::AdjustWndSize(int expectWidth, int expectHeight)
{
    int finalWidth;
    int finalHeight;
    LONG minWidth;
    LONG minHeight;
    GetMinWinSize(minWidth, minHeight);
    if (m_bShowTreeMain) {
        // 有树控件的情况下根据当前尺寸和最小尺寸对比判断
        RECT rcNow;
        GetWindowRect(&rcNow);
        LONG nowWidth = rcNow.right - rcNow.left;
        LONG nowHeight = rcNow.bottom - rcNow.top;

        if (expectWidth > 0) {
            finalWidth = (expectWidth < minWidth) ? minWidth : expectWidth;
        } else {
            finalWidth = (nowWidth < minWidth) ? minWidth : nowWidth;
        }
        if (expectHeight > 0) {
            finalHeight = (expectHeight < minHeight) ? minHeight : expectHeight;
        } else {
            finalHeight = (nowHeight < minHeight) ? minHeight : nowHeight;
        }
    } else {
        // 没有树控件的情况下强制调整到最小尺寸
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
    }
    // SWP_FRAMECHANGED 会触发 WM_NCCALCSIZE, 进而让系统查询 WM_GETMINMAXINFO
    SetWindowPos(NULL, 0, 0, finalWidth, finalHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

    // 更新整体布局
    RECT rcClient;
    GetClientRect(&rcClient);
    UpdateLayout(rcClient.right, rcClient.bottom);
}

LRESULT MainWindow::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    HDC hdc = (HDC)wParam;
    HWND hwndLabel = (HWND)lParam;

    if (hwndLabel == m_labelIMExtInstallStatus.m_hWnd) {
        SetTextColor(hdc, m_labelIMExtInstallStatusColor);
        SetBkMode(hdc, TRANSPARENT); // 透明背景
        return (LRESULT)GetStockObject(NULL_BRUSH); // 需要注意更新文字时重绘父窗口
    }
    if (hwndLabel == m_labelProgress.m_hWnd) {
        SetTextColor(hdc, m_labelProgressColor);
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    
    return 0;
}

void MainWindow::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == IDT_UPDATE_STATIC_TIMER) {
        UpdateAllStaticControls();
    }
}

void MainWindow::OnLButtonDown(UINT nFlags, CPoint point) {
    // 点击空白处将焦点设置给主窗口
    SetFocus(); 
    
    // 点击后仍能处理其他消息
    SetMsgHandled(FALSE); 
}

LRESULT MainWindow::OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    // wParam 是触发右键事件的窗口句柄
    HWND hWndTarget = (HWND)wParam;
    
    // 判断属于安装IMEXT按钮
    if (hWndTarget == GetDlgItem(IDC_BTN_INSTALL_IMEXT)) {
        // 获取当前鼠标的屏幕坐标（lParam 包含了坐标，如果通过键盘触发则为 -1）
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        CreateToolBar(pt);
        return 0;
    }

    bHandled = FALSE; // 不是目标按钮，交由系统默认处理
    return 0;
}

void MainWindow::CreateToolBar(POINT pt) {
    CMenu menu;
    menu.CreatePopupMenu();

    menu.AppendMenu(MF_STRING, IDM_RWR_FONT_RESIZE_TOOL, _TR(IDS_RWR_FONT_RESIZE_TOOL));

    int cmd = menu.TrackPopupMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
    
    switch (cmd)
    {
    case IDM_RWR_FONT_RESIZE_TOOL:
    {
        FontResizeToolDialog fdlg(m_rwrInstallPath);
        fdlg.DoModal();
        break;
    }
    default:
        break;
    }
}

void MainWindow::UpdateAllStaticControls() {
    LabelDrawInfo* pIMExtInstallStatusText = (LabelDrawInfo*)InterlockedExchangePointer((void **)&m_pPendingIMExtInstallStatusText, NULL);
    if (pIMExtInstallStatusText) {
        m_labelIMExtInstallStatusText = pIMExtInstallStatusText->text;
        m_labelIMExtInstallStatusColor = pIMExtInstallStatusText->color;
        m_labelIMExtInstallStatus.SetWindowText(m_labelIMExtInstallStatusText);

        RECT rc;
        m_labelIMExtInstallStatus.GetWindowRect(&rc);
        ScreenToClient(&rc);
        InvalidateRect(&rc, TRUE);

        delete pIMExtInstallStatusText;
    }

    LabelDrawInfo* pProgressText = (LabelDrawInfo*)InterlockedExchangePointer((void **)&m_pPendingProgressText, NULL);
    if (pProgressText) {
        m_labelProgressText = pProgressText->text;
        m_labelProgressColor = pProgressText->color;
        m_labelProgress.SetWindowText(m_labelProgressText);
        
        RECT rc;
        m_labelProgress.GetWindowRect(&rc);
        ScreenToClient(&rc);
        InvalidateRect(&rc, TRUE);

        AdjustWndSize();

        delete pProgressText;
    }
}

void MainWindow::SetInstallStatusText(const CString& text, COLORREF color)
{
    // 1. 准备新数据
    LabelDrawInfo* pNew = new LabelDrawInfo{text, color};
    
    // 2. 原子交换: 将label更新信息的指针传入
    LabelDrawInfo* pOld = (LabelDrawInfo*)InterlockedExchangePointer((void **)&m_pPendingIMExtInstallStatusText, pNew);
    
    // 3. 立刻释放旧指针
    if (pOld) delete pOld;
}

void MainWindow::SetProgressText(const CString& text, COLORREF color)
{
    // 1. 准备新数据
    LabelDrawInfo* pNew = new LabelDrawInfo{text, color};
    
    // 2. 原子交换: 将label更新信息的指针传入
    LabelDrawInfo* pOld = (LabelDrawInfo*)InterlockedExchangePointer((void **)&m_pPendingProgressText, pNew);
    
    // 3. 立刻释放旧指针
    if (pOld) delete pOld;
}

void MainWindow::UpdateMD5CheckProgress(int progressNum, int totalNum)
{
    CString labelProgressText;
    labelProgressText.Format(
        L"%s: %d / %d",
        _TR(IDS_MD5_CHECK_PROGRESS),
        progressNum,
        totalNum
    );
    SetProgressText(labelProgressText, MY_COLOR_DEFAULT);
}

LRESULT MainWindow::OnUpdateTreeMain(UINT, WPARAM, LPARAM, BOOL&)
{
    AdjustWndSize();
    return 0;
}

LRESULT MainWindow::OnSelectRwrInstallPath(WORD /*wParam*/, WORD /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/)
{
    CString rwrPath;
    HRESULT hr = FileUtils::SelectFolderInDialog(rwrPath, _TR(IDS_SELECT_RWR_PATH), m_hWnd);
    if (SUCCEEDED(hr))
    {
        if (!rwrPath.IsEmpty() && (FileUtils::GetRealPath(m_rwrInstallPath) != FileUtils::GetRealPath(rwrPath)))
        {
            UpdateRwrInstallPath(rwrPath);
        }
    }
    return hr;
}

LRESULT MainWindow::OnSelectBackupPath(WORD /*wParam*/, WORD /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/)
{
    CString backupPath;
    HRESULT hr = FileUtils::SelectFolderInDialog(backupPath, _TR(IDS_SELECT_BACKUP_PATH), m_hWnd);
    if (SUCCEEDED(hr))
    {
        if (!backupPath.IsEmpty())
        {
            m_backupPath = backupPath;
            m_editBackupPath.SetWindowText(m_backupPath);
        }
    }
    return hr;
}

LRESULT MainWindow::OnEditKillFocus(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
{
    if (hWndCtl == m_editRwrInstallPath.m_hWnd) {
        CString rwrPath;
        m_editRwrInstallPath.GetWindowText(rwrPath);
        rwrPath = FileUtils::NormalizePath(rwrPath);
        if (FileUtils::GetRealPath(m_rwrInstallPath) != FileUtils::GetRealPath(rwrPath)) {
            UpdateRwrInstallPath(rwrPath);
        } else {
            m_editRwrInstallPath.SetWindowText(m_rwrInstallPath);
        }
    }

    if (hWndCtl == m_editBackupPath.m_hWnd) {
        CString backupPath;
        m_editBackupPath.GetWindowText(backupPath);
        backupPath = FileUtils::NormalizePath(backupPath);
        m_backupPath = backupPath; // 手动编辑的情况下接受空白路径
        m_editBackupPath.SetWindowText(m_backupPath);
    }

    return 0;
}

void MainWindow::UpdateRwrInstallPath(const CString& rwrPath)
{
    m_rwrInstallPath = rwrPath;
    m_editRwrInstallPath.SetWindowText(rwrPath);
    FileTreeView::SetRwrInstallPath(m_rwrInstallPath);

    if (FileUtils::FileExists(m_rwrInstallPath + _T("\\") + ImextVerifier::strRwrGameExe)) {
        StartCheckImextInstallStatus();
    } else {
        SetInstallStatusText(_TR(IDS_NOT_RWR_PATH), MY_COLOR_ERROR);
        SetProgressText(L"");
        m_btnSelectBackupPath.EnableWindow(FALSE);
        m_btnInstallIMExt.EnableWindow(FALSE);
        m_bShowTreeMain = false;
        InitTreeNodeDataRoot();
        SendMessageW(WM_USER_UPDATE_TREE_MAIN);
    }
}

LRESULT MainWindow::OnInstallImext(WORD /*wParam*/, WORD /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/)
{
    
    StartInstallImext();
    return 0;
}

void MainWindow::StartCheckImextInstallStatus()
{
    if (m_hCheckInstallThread != NULL) {
        // 如果线程还在跑，无限期等待直到它结束
        WaitForSingleObject(m_hCheckInstallThread, INFINITE);
        CloseHandle(m_hCheckInstallThread);
        m_hCheckInstallThread = NULL;
        m_nMaxMD5CheckProgress = -1;
    }

    // 禁用所有按钮并隐藏tree组件
    m_btnSelectRwrInstallPath.EnableWindow(FALSE);
    m_btnSelectBackupPath.EnableWindow(FALSE);
    m_btnInstallIMExt.EnableWindow(FALSE);
    m_bShowTreeMain = false;
    // 重置节点数据
    InitTreeNodeDataRoot();
    SendMessageW(WM_USER_UPDATE_TREE_MAIN);

    m_hCheckInstallThread = (HANDLE)_beginthreadex(NULL, 0, &CheckImextInstallStatusThreadEntry, this, 0, NULL);
}

void MainWindow::StartInstallImext()
{
    if (m_hInstallImextThread != NULL) {
        // 如果线程还在跑，无限期等待直到它结束
        WaitForSingleObject(m_hInstallImextThread, INFINITE);
        CloseHandle(m_hInstallImextThread);
        m_hInstallImextThread = NULL;
        m_nMaxMD5CheckProgress = -1;
    }
    if (m_hCheckInstallThread != NULL) {
        // 如果线程还在跑，无限期等待直到它结束
        WaitForSingleObject(m_hCheckInstallThread, INFINITE);
        CloseHandle(m_hCheckInstallThread);
        m_hCheckInstallThread = NULL;
        m_nMaxMD5CheckProgress = -1;
    }

    // 禁用所有按钮
    m_btnSelectRwrInstallPath.EnableWindow(FALSE);
    m_btnSelectBackupPath.EnableWindow(FALSE);
    m_btnInstallIMExt.EnableWindow(FALSE);

    m_hInstallImextThread = (HANDLE)_beginthreadex(NULL, 0, &InstallImextThreadEntry, this, 0, NULL);
}

void MainWindow::CheckImextInstallStatus()
{
    SetInstallStatusText(_TR(IDS_CHECK_INSTALLATION), MY_COLOR_DEFAULT);

    // 检查当前目录安装状态
    CString statusText;
    m_installStatus = m_imextVerifier.CheckInstallVersion(m_rwrInstallPath, m_installedVer);
    if (!m_installStatus) {
        statusText = _TR(IDS_INSTALL_STATUS_NOT);
        SetInstallStatusText(statusText, MY_COLOR_ERROR);
    } else {
        m_installedVerCompareRet = m_imextVerifier.VersionCompareFull(m_installedVer);
        if (m_installedVerCompareRet == 0) {
            statusText.Format(L"%s %s", _TR(IDS_INSTALL_STATUS_LATEST), m_installedVer);
            SetInstallStatusText(statusText, MY_COLOR_SUCCESS);
        } else if (m_installedVerCompareRet == -1) {
            statusText.Format(L"%s %s", _TR(IDS_INSTALL_STATUS_OLD), m_installedVer);
            SetInstallStatusText(statusText, MY_COLOR_CRITICAL);
        } else {
            statusText.Format(L"%s %s", _TR(IDS_INSTALL_STATUS_NEW), m_installedVer);
            SetInstallStatusText(statusText, MY_COLOR_CRITICAL);
        }
    }

    // 检查当前目录安装版本的编译时间, 没有时间戳Tag的版本统一当成第一次发布的时间
    m_installTimestamp = m_imextVerifier.CheckInstallTimestamp(m_rwrInstallPath);

    // 检查文件MD5并展示文件树
    const cJSON* checkResult = m_imextVerifier.CheckFileMD5(m_rwrInstallPath);
    cJSON* result = checkResult->child;
    while (result) {
        if (cJSON_IsObject(result) && result->child) {
            COLORREF resultColor = MY_COLOR_DEFAULT;
            bool bAddTimestamp = false;
            if (strcmp(_KU8(IDS_KEY_CHECK_RESULT_MISS_FILE), result->string) == 0) {
                resultColor = MY_COLOR_ERROR;
            } else if (strcmp(_KU8(IDS_KEY_CHECK_RESULT_DIFF_FILE), result->string) == 0) {
                resultColor = MY_COLOR_CRITICAL;
            } else if (strcmp(_KU8(IDS_KEY_CHECK_RESULT_ORIG_FILE), result->string) == 0) {
                resultColor = MY_COLOR_SUCCESS_DARK;
                bAddTimestamp = true;
            } else if (strcmp(_KU8(IDS_KEY_CHECK_RESULT_SAME_FILE), result->string) == 0) {
                resultColor = MY_COLOR_SUCCESS;
            }

            CString resultName(_K2TR(result->string));
            if (bAddTimestamp) {
                SYSTEMTIME stLocal;
                if (UnixTimeToLocalSystemTime(m_imextVerifier.m_lastGameUpdateTimestamp, stLocal)) {
                    resultName += GetFormattedTime(stLocal, L" (%04d-%02d-%02d %02d:%02d:%02d)");
                }
            }
            AddCheckResultTreeNodeData(resultName, result, resultColor); // resultName需要加翻译
        }
        result = result->next;
    }
    m_bShowTreeMain = true;
    PostMessageW(WM_USER_UPDATE_TREE_MAIN); // 后面不能继续在子线程对节点进行任何操作

    // 检查游戏兼容性
    m_bGameCompatibility = m_imextVerifier.CheckGameCompatibility(m_rwrInstallPath);
    // 不能单纯依靠exe md5来判断兼容性, 即使MD5和配置不一致只能说明游戏自己更新了EXE或者是插件自己更新了或者是被其他东西改了
    switch (m_bGameCompatibility)
    {
    case ImextVerifier::GameCompatibility::IMExtTagExe:
    {
        // MD5不一致就是相当于版本不一致
        statusText.Format(L"%s - %s", statusText, _TR(IDS_MISMATCHED_VERSION));
        SetInstallStatusText(statusText, MY_COLOR_CRITICAL);
        break;
    }
    case ImextVerifier::GameCompatibility::UnknownExe:
    {
        statusText.Format(L"%s - %s", statusText, _TR(IDS_GAME_EXE_STATUS_ERROR));
        SetInstallStatusText(statusText, MY_COLOR_CRITICAL);
        break;
    }
    default:
        break;
    }

    m_btnSelectBackupPath.EnableWindow(TRUE);
    m_btnInstallIMExt.EnableWindow(TRUE);

    // 恢复更新游戏路径按钮状态
    SetProgressText(L"");
    m_btnSelectRwrInstallPath.EnableWindow(TRUE);
}

void MainWindow::InstallImext()
{
    SetInstallStatusText(_TR(IDS_INSTALLING), MY_COLOR_DEFAULT);

    // 从低于1.2.0的旧版本升级，并且当前待安装版本大于等于1.2.0，需要提示恢复文件
    // 仅在近期版本维护主动恢复文件策略，后续删除除了EXE以外的官方文件备份
    if (!m_installedVer.IsEmpty() && m_imextVerifier.VersionCompareXYZ(m_imextVerifier.m_version, _T("1.2.0")) >= 0 && m_imextVerifier.VersionCompareXYZ(m_installedVer, _T("1.2.0")) == -1) {
        bool retUpgradeFrom110To120 = ShowModalAskDialog(
            m_hWnd,
            _TR(IDS_TITLE_UPGRADE_FROM_110_TO_120),
            _TR(IDS_DESC_UPGRADE_FROM_110_TO_120)
        );
        if (retUpgradeFrom110To120) {
            // 获取原版文件备份文件夹下的所有文件
            CSimpleArray<CString> officialBackupRelPathList;
            if (!FileUtils::GetFileRelPathListByMask(m_officialBackupPath, _T("**"), officialBackupRelPathList) || officialBackupRelPathList.GetSize() == 0) {
                CString errorText;
                errorText.Format(L"%s: %s",
                    _TR(IDS_ERROR_DESC_GET_OFFICIAL_BACKUP), m_officialBackupPath);
                ShowModalError(
                    m_hWnd,
                    _TR(IDS_TITLE_ERROR),
                    errorText
                );
                InitTreeNodeDataRoot();
                CheckImextInstallStatus();
                SetProgressText(_TR(IDS_CANCEL_INSTALL), MY_COLOR_ERROR);
                return;
            }

            // 恢复文件
            int ret = FileUtils::CopyFilesByFileRelPathWithMakedirs(
                m_officialBackupPath, m_rwrInstallPath,
                officialBackupRelPathList,
                CopyFilesProgressCallback,
                CopyFilesErrorCallback,
                this,
                true
            );
            if (!ret) {
                // 检查安装状态并退出
                InitTreeNodeDataRoot();
                CheckImextInstallStatus();
                SetProgressText(_TR(IDS_BACKUP_ERROR), MY_COLOR_ERROR);
                return;
            }

            // 删除旧的插件文件
            CSimpleArray<CString> deleteRelPathList;
            deleteRelPathList.Add(L"imext_config.ini");
            deleteRelPathList.Add(L"IMEUiEmoji.ttf");
            deleteRelPathList.Add(L"IMEUiText.otf");
            CString deleteDescText;
            deleteDescText.Format(L"%s\n%s", _TR(IDS_DESC_OFFICIAL_BACKUP_RECOVERY_SUCCEED), _TR(IDS_DESC_DELETE_OLD_FILE));
            bool retDeleteOldFile = ShowModalAskDialog(
                m_hWnd,
                _TR(IDS_TITLE_CONFIRM),
                _TR(IDS_DESC_DELETE_OLD_FILE),
                &deleteRelPathList,
                m_rwrInstallPath,
                m_imextPath
            );
            if (retDeleteOldFile) {
                FileUtils::RemoveFilesByFileRelPath(m_rwrInstallPath, deleteRelPathList, false);
                ShowModalInfo(
                    m_hWnd,
                    _TR(IDS_TITLE_CONFIRM),
                    _TR(IDS_DESC_DELETE_OLD_FILE_SUCCEED)
                );
            }
        }
    }

    // 老系统提示
    if (!IsWindows10OrGreater()) {
        bool retOldWin = ShowModalAskDialog(
            m_hWnd,
            _TR(IDS_TITLE_NOTICE),
            _TR(IDS_OLD_WIN_DESC)
        );
        if (!retOldWin) {
            InitTreeNodeDataRoot();
            CheckImextInstallStatus();
            SetProgressText(_TR(IDS_CANCEL_INSTALL), MY_COLOR_ERROR);
            return;
        }
    }

    // 兼容异常提示
    if (m_bGameCompatibility == ImextVerifier::GameCompatibility::UnknownExe) {
        // 获取插件对应游戏EXE的更新日期
        SYSTEMTIME stLocal;
        CString updateTimeFormated = m_imextVerifier.m_lastGameUpdateTimestampStr;
        if (UnixTimeToLocalSystemTime(m_imextVerifier.m_lastGameUpdateTimestamp, stLocal)) {
            updateTimeFormated = GetFormattedTime(stLocal, L"%04d-%02d-%02d %02d:%02d:%02d");
        }
        CString unknownExeDescText;
        unknownExeDescText.Format(L"%s\n%s: %s\n%s: %s", 
            _TR(IDS_UNKNOWN_EXE_WARNING_DESC),
            _TR(IDS_KEY_GAME_VERSION), m_imextVerifier.m_lastGameUpdateVersion,
            _TR(IDS_KEY_LAST_GAME_UPDATE_TIMESTAMP), updateTimeFormated
        ); // 不直接写是为了方便翻译
        bool retUnknownExe = ShowModalAskDialog(
            m_hWnd,
            _TR(IDS_TITLE_WARNING),
            unknownExeDescText,
            (cJSON*)nullptr, nullptr, nullptr, IDI_WARNING
        );
        if (!retUnknownExe) {
            InitTreeNodeDataRoot();
            CheckImextInstallStatus();
            SetProgressText(_TR(IDS_CANCEL_INSTALL), MY_COLOR_ERROR);
            return;
        }
    }

    // 已安装版本号更高提示
    if (!m_installedVer.IsEmpty() && m_installedVerCompareRet > 0) {
        bool retOverwriteInstall = ShowModalAskDialog(
            m_hWnd,
            _TR(IDS_TITLE_NOTICE),
            _TR(IDS_NEWER_VERSION_OVERWRITE_INSTALL_DESC)
        );
        if (!retOverwriteInstall) {
            InitTreeNodeDataRoot();
            CheckImextInstallStatus();
            SetProgressText(_TR(IDS_CANCEL_INSTALL), MY_COLOR_ERROR);
            return;
        }
    }

    // 已安装编译时间更晚提示
    if (m_imextVerifier.m_timestamp < m_installTimestamp) {
        SYSTEMTIME stLocalImext;
        CString imextTimeFormated = m_imextVerifier.m_timestampStr;
        if (UnixTimeToLocalSystemTime(m_imextVerifier.m_timestamp, stLocalImext)) {
            imextTimeFormated = GetFormattedTime(stLocalImext, L"%04d-%02d-%02d %02d:%02d:%02d");
        }
        SYSTEMTIME stLocalInstall;
        CString installTimeFormated;
        if (UnixTimeToLocalSystemTime(m_installTimestamp, stLocalInstall)) {
            installTimeFormated = GetFormattedTime(stLocalInstall, L"%04d-%02d-%02d %02d:%02d:%02d");
        }
        CString patchTimeDescText;
        patchTimeDescText.Format(L"%s\n%s: %s\n%s: %s",
            _TR(IDS_PATCH_TIME_NEWER_DESC),
            _TR(IDS_IMEXT_PATCH_TIME), imextTimeFormated,
            _TR(IDS_INSTALLED_PATCH_TIME), installTimeFormated
        );
        bool retPatchTime = ShowModalAskDialog(
            m_hWnd,
            _TR(IDS_TITLE_NOTICE),
            patchTimeDescText
        );
        if (!retPatchTime) {
            InitTreeNodeDataRoot();
            CheckImextInstallStatus();
            SetProgressText(_TR(IDS_CANCEL_INSTALL), MY_COLOR_ERROR);
            return;
        }
    }

    // 准备数据
    SetProgressText(_TR(IDS_PREPARE_FILE_LIST));
    CSimpleArray<CString> backupFileRelPathList;
    CSimpleArray<CString> installImextFileList;
    cJSON* categoryFileRelPathInfo = m_imextVerifier.m_imextFileRelPathMD5InfoSorted->child;
    while (categoryFileRelPathInfo) {
        if (
            strcmp(categoryFileRelPathInfo->string, _KU8(IDS_KEY_CATEGORY_IMEXT_RESOURCES)) == 0
        ) {
            cJSON* childfileRelPath = categoryFileRelPathInfo->child;
            while (childfileRelPath) {
                CString fileRelPath(CA2W(childfileRelPath->string, CP_UTF8));
                backupFileRelPathList.Add(fileRelPath); // Imext Resources也要加入到里面防止用户需要整个恢复旧版本插件
                installImextFileList.Add(fileRelPath);
                childfileRelPath = childfileRelPath->next;
            }
        } else if (strcmp(categoryFileRelPathInfo->string, _KU8(IDS_KEY_CATEGORY_GAME_EXE)) == 0) {
            cJSON* childfileRelPath = categoryFileRelPathInfo->child;
            while (childfileRelPath) {
                CString fileRelPath(CA2W(childfileRelPath->string, CP_UTF8));
                backupFileRelPathList.Add(fileRelPath);
                installImextFileList.Add(fileRelPath);
                childfileRelPath = childfileRelPath->next;
            }
        }
        categoryFileRelPathInfo = categoryFileRelPathInfo->next;
    }

    // 备份文件
    SetProgressText(_TR(IDS_PREPARE_BACKUP));
    CString backupFolderPath = FileUtils::NormalizePath(m_backupPath + L"\\" + GetFormattedTime(L"%04d-%02d-%02d_%02d-%02d-%02d"));
    bool retBackup = ShowModalAskDialog(
        m_hWnd,
        _TR(IDS_TITLE_BACKUP),
        _TR(IDS_BACKUP_DESC) + CString(L"\n") + backupFolderPath
    );
    if (retBackup) {
        int ret = FileUtils::CopyFilesByFileRelPathWithMakedirs(
            m_rwrInstallPath, backupFolderPath,
            backupFileRelPathList,
            CopyFilesProgressCallback,
            CopyFilesErrorCallback,
            this,
            true
        );
        if (!ret) {
            // 检查安装状态并退出
            InitTreeNodeDataRoot();
            CheckImextInstallStatus();
            SetProgressText(_TR(IDS_BACKUP_ERROR), MY_COLOR_ERROR);
            return;
        }
    }

    CString installLabelProgressText;
    // 安装Imext Resources + Game Exe
    installLabelProgressText.Format(L"%s", _TR(IDS_INSTALL_IMEXT)); // 不直接写是为了方便翻译
    SetProgressText(installLabelProgressText, MY_COLOR_DEFAULT);
    bool retInstallImext = ShowModalAskDialog(
        m_hWnd,
        installLabelProgressText,
        _TR(IDS_INSTALL_IMEXT_FILES_DESC) + CString(L"\n") + m_rwrInstallPath,
        &installImextFileList,
        m_rwrInstallPath,
        m_imextPath
    );
    if (retInstallImext) {
        int ret = FileUtils::CopyFilesByFileRelPathWithMakedirs(
            m_imextPath, m_rwrInstallPath,
            installImextFileList,
            CopyFilesProgressCallback,
            CopyFilesErrorCallback,
            this,
            true
        );
        if (!ret) {
            // 检查安装状态并退出
            InitTreeNodeDataRoot();
            CheckImextInstallStatus();
            SetProgressText(_TR(IDS_INSTALL_IMEXT_ERROR), MY_COLOR_ERROR);
            return;
        }
    }

    // 检测当前游戏exe是否开启了高DPI设置
    CString gameExePath = m_rwrInstallPath + _T("\\") + ImextVerifier::strRwrGameExe;
    CString gameExeAppCompatFlags;
    if (RegeditUtils::GetAppCompatFlags(gameExePath, gameExeAppCompatFlags)) {
        gameExeAppCompatFlags.Trim();
        CSimpleArray<CString> flagList = StringUtils::Split(gameExeAppCompatFlags);
        CString pureFlags;
        const int count = flagList.GetSize();
        bool hasDpiFlags = false;
        for (int i = 0; i < count; i++) {
            if (flagList[i] == L"PERPROCESSSYSTEMDPIFORCEOFF") { hasDpiFlags = true; continue; }
            if (flagList[i] == L"PERPROCESSSYSTEMDPIFORCEON") { hasDpiFlags = true; continue; }
            if (flagList[i] == L"HIGHDPIAWARE") { hasDpiFlags = true; continue; }
            if (flagList[i] == L"DPIUNAWARE") { hasDpiFlags = true; continue; }
            if (flagList[i] == L"GDIDPISCALING") { hasDpiFlags = true; continue; }
            pureFlags += flagList[i] + L" ";
        }
        pureFlags.Trim();

        if (hasDpiFlags) {
            bool retDelDpiFlags = ShowModalAskDialog(
                m_hWnd,
                _TR(IDS_TITLE_NOTICE),
                _TR(IDS_DESC_DELETE_DPI_FLAGS)
            );
            if (retDelDpiFlags) {
                if (!RegeditUtils::SetAppCompatFlags(gameExePath, pureFlags)) {
                    InitTreeNodeDataRoot();
                    CheckImextInstallStatus();
                    SetProgressText(_TR(IDS_DELETE_DPI_FLAGS_ERROR), MY_COLOR_ERROR);
                    return;
                }
            }
        }
    }

    // 检测屏幕分辨率
    MonitorUtils::ScreenResolution primaryScreenResolution;
    CSimpleArray<MonitorUtils::ScreenResolution> allScreenResolutionList;
    bool resolutionFetched = true;
    do {
        if (!MonitorUtils::GetPrimaryScreenResolution(primaryScreenResolution.nWidth, primaryScreenResolution.nHeight)) {
            resolutionFetched = false;
            break;
        }
        if (!MonitorUtils::GetAllScreenResolutionList(allScreenResolutionList)) {
            resolutionFetched = false;
            break;
        }
    } while (false);
    
    // 检测游戏配置文件分辨率
    if (resolutionFetched) {
        MonitorUtils::ScreenResolution configResolution;
        if (XmlUtils::ReadConfigVideoModeResolution(m_rwrConfigPath, configResolution.nWidth, configResolution.nHeight)) {
            if (configResolution != primaryScreenResolution) {
                const int screenCount = allScreenResolutionList.GetSize();
                CString allScreenResolutionStr;
                for (int i = 0; i < screenCount; i++) {
                    CString resFormatter;
                    resFormatter.Format(L"%dx%d", allScreenResolutionList[i].nWidth, allScreenResolutionList[i].nHeight);
                    if (i == (screenCount - 1))
                        allScreenResolutionStr += resFormatter;
                    else
                        allScreenResolutionStr += resFormatter + L", ";
                }
                CString editConfigResolution;
                editConfigResolution.Format(L"%s\n%s: %dx%d\n%s: %dx%d\n%s: %s\n\n%s (%dx%d)?",
                    _TR(IDS_DESC_CONFIG_RESOLUTION_MISMATCH),
                    _TR(IDS_DESC_CONFIG_RESOLUTION), configResolution.nWidth, configResolution.nHeight,
                    _TR(IDS_DESC_PRIMARY_RESOLUTION), primaryScreenResolution.nWidth, primaryScreenResolution.nHeight,
                    _TR(IDS_DESC_ALL_RESOLUTION), allScreenResolutionStr,
                    _TR(IDS_DESC_EDIT_CONFIG_RESOLUTION), primaryScreenResolution.nWidth, primaryScreenResolution.nHeight
                );
                bool retChangeRes = ShowModalAskDialog(
                    m_hWnd,
                    _TR(IDS_TITLE_NOTICE),
                    editConfigResolution
                );
                if (retChangeRes) {
                    if (!XmlUtils::WriteConfigVideoModeResolution(m_rwrConfigPath, primaryScreenResolution.nWidth, primaryScreenResolution.nHeight)) {
                        InitTreeNodeDataRoot();
                        CheckImextInstallStatus();
                        SetProgressText(_TR(IDS_EDIT_CONFIG_RESOLUTION_ERROR), MY_COLOR_ERROR);
                        return;
                    }
                }
            }
        }
    }
    

    /* // 不再需要进行DX9确认
    // 检查config
    if (m_rwrConfigManager.LoadConfig(m_rwrConfigPath)) {
        CString rendersystem = m_rwrConfigManager.GetValue(L"rendersystem");
        if (rendersystem.CompareNoCase(L"directx") == 0) {
            ShowModalWarning(
                m_hWnd,
                _TR(IDS_TITLE_NOTICE),
                _TR(IDS_DX9_NOTICE)
            );
        }
    }
    */

    /* // 不再提供字体分辨率修改提示
    CString fontResizeNoticeText;
    fontResizeNoticeText.Format(L"%s\n%s\n \n%s\n \n%s",
        _TR(IDS_RWR_FONT_PROBLEM_NOTICE_A),
        _TR(IDS_RWR_FONT_PROBLEM_DESC),
        _TR(IDS_RWR_FONT_PROBLEM_NOTICE_B),
        _TR(IDS_RWR_FONT_PROBLEM_NOTICE_C)
    );
    ShowModalInfo(
        m_hWnd,
        _TR(IDS_TITLE_NOTICE),
        fontResizeNoticeText
    ); */

    // 提示ini位置确定打开, 后续可以增加ini的字段校验和新增没有的字段的功能
    CString iniNoticeText;
    iniNoticeText.Format(L"%s\n%s",
        _TR(IDS_IMEXT_YAML_CONFIG_PATH_NOTICE),
        FileUtils::NormalizePath(m_rwrInstallPath + L"\\" + ImextVerifier::strImextConfigYaml)
    );
    bool retIniOpen = ShowModalAskDialog(
        m_hWnd,
        _TR(IDS_TITLE_NOTICE),
        iniNoticeText
    );
    if (retIniOpen) {
        FileUtils::OpenFile(m_rwrInstallPath + L"\\" + ImextVerifier::strImextConfigYaml);
    }

    // 检查安装状态
    InitTreeNodeDataRoot();
    CheckImextInstallStatus();
    if (retInstallImext) {
        SetProgressText(_TR(IDS_INSTALL_SUCCESS), MY_COLOR_SUCCESS);
    } else {
        SetProgressText(_TR(IDS_INSTALL_SUCCESS_ALT), MY_COLOR_SUCCESS);
    }
}

void MainWindow::AddCheckResultTreeNodeData(LPCTSTR resultName, const cJSON* const sortedFileRelPathInfo, COLORREF color, LPCTSTR overwriteAdditionalText)
{
    FileTreeNodeData* newResultDataRoot = new FileTreeNodeData(); // 此处new不用手动释放
    newResultDataRoot->SetColor(color).SetOpen(true);
    m_treeNodeDataRoot.AddSubNode(resultName, newResultDataRoot);

    if (!sortedFileRelPathInfo) return;
    cJSON* categoryFileRelPathInfo = sortedFileRelPathInfo->child;
    while (categoryFileRelPathInfo) {
        if (cJSON_IsObject(categoryFileRelPathInfo)) {
            CString categoryName(_K2TR(categoryFileRelPathInfo->string));
            FileTreeNodeData* newCategoryNodeData = new FileTreeNodeData();
            newCategoryNodeData->SetColor(color).SetOpen(false);
            newResultDataRoot->AddSubNode(categoryName, newCategoryNodeData);
            FileTreeView::insertFileRelPathNode(newCategoryNodeData, categoryFileRelPathInfo, color, overwriteAdditionalText);
        }
        categoryFileRelPathInfo = categoryFileRelPathInfo->next;
    }
}

void MainWindow::MD5CheckProgressCallback(int progressNum, int totalNum, void* pCaller)
{
    if (pCaller) {
        MainWindow* pThis = static_cast<MainWindow*>(pCaller);

        if (progressNum < pThis->m_nMaxMD5CheckProgress) {
            return;
        }
        // 由于回调调用速度极快, 并且需要兼容通用的更新label方法, 只能用加锁解决, 需保证内部只有瞬时操作
        pThis->m_csMD5CheckProgress.Lock();
        if (progressNum >= pThis->m_nMaxMD5CheckProgress) 
        {
            pThis->m_nMaxMD5CheckProgress = progressNum;
            pThis->UpdateMD5CheckProgress(progressNum, totalNum);
        }
        pThis->m_csMD5CheckProgress.Unlock();
    }
}

void MainWindow::ResetMD5CheckProgressCallback(void* pCaller)
{
    if (pCaller) {
        MainWindow* pThis = static_cast<MainWindow*>(pCaller);
        pThis->m_nMaxMD5CheckProgress = -1;
    }
}

void MainWindow::CopyFilesProgressCallback(int progressNum, int totalNum, void* pCaller)
{
    if (pCaller) {
        MainWindow* pThis = static_cast<MainWindow*>(pCaller);
        // 此处不涉及多子线程复制可以不用加锁
        CString labelProgressText;
        labelProgressText.Format(
            L"%s: %d / %d",
            _TR(IDS_TITLE_COPY),
            progressNum,
            totalNum
        );
        pThis->SetProgressText(labelProgressText, MY_COLOR_DEFAULT);
    }
}

void MainWindow::CopyFilesErrorCallback(LPCTSTR error, void* pCaller)
{
    if (pCaller) {
        MainWindow* pThis = static_cast<MainWindow*>(pCaller);
        ShowModalError(
            pThis->m_hWnd,
            _TR(IDS_TITLE_ERROR),
            error
        );
    }
}