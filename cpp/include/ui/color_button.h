#pragma once
#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlstr.h>
#include <atlcrack.h>
#include <atltypes.h>

class CColorButton : public CWindowImpl<CColorButton, CButton> {
public:
    COLORREF m_clrBack;
    COLORREF m_clrText;
    bool m_bMouseOver; // 追蹤鼠標是否懸浮

    CColorButton() : 
        m_clrBack(RGB(0, 120, 215)), 
        m_clrText(RGB(255, 255, 255)),
        m_bMouseOver(false)
    {}

    void SetColors(COLORREF clrBack, COLORREF clrText) {
        m_clrBack = clrBack;
        m_clrText = clrText;
        if (IsWindow()) Invalidate();
    }

    BEGIN_MSG_MAP(CColorButton)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
        MSG_OCM_DRAWITEM(OnReflectedDrawItem) // 反射的自绘消息
    END_MSG_MAP()

    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
        ModifyStyle(0, BS_OWNERDRAW); // 自动添加自绘属性
        return 0;
    }

    LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
        if (!m_bMouseOver) {
            m_bMouseOver = true;
            
            // 追踪鼠标离开消息
            TRACKMOUSEEVENT tme = { sizeof(tme) };
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hWnd;
            ::_TrackMouseEvent(&tme);
            
            Invalidate(); // 触发重绘显示颜色
        }
        bHandled = FALSE;
        return 0;
    }

    LRESULT OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
        m_bMouseOver = false;
        Invalidate(); // 恢复原色
        return 0;
    }

    void OnReflectedDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
        CDCHandle dc = lpDrawItemStruct->hDC;
        CRect rc = lpDrawItemStruct->rcItem;
        UINT state = lpDrawItemStruct->itemState;

        // 1. 状态判断
        bool bDisabled = (state & ODS_DISABLED);
        bool bSelected = (state & ODS_SELECTED);

        // 2. 计算颜色
        COLORREF clrCurrentBack;
        COLORREF clrCurrentText;

        if (bDisabled) {
            // 禁用状态设置灰色
            clrCurrentBack = RGB(204, 204, 204); 
            clrCurrentText = RGB(140, 140, 140);
        }
        else if (bSelected) {
            // 按下状态
            clrCurrentBack = RGB(GetRValue(m_clrBack) * 0.7, GetGValue(m_clrBack) * 0.7, GetBValue(m_clrBack) * 0.7);
            clrCurrentText = m_clrText;
        }
        else if (m_bMouseOver) {
            // 悬浮状态
            clrCurrentBack = RGB(GetRValue(m_clrBack) * 0.9, GetGValue(m_clrBack) * 0.9, GetBValue(m_clrBack) * 0.9);
            clrCurrentText = m_clrText;
        }
        else {
            // 正常状态
            clrCurrentBack = m_clrBack;
            clrCurrentText = m_clrText;
        }

        // 3. 绘制背景(不带边框)
        CBrush br;
        br.CreateSolidBrush(clrCurrentBack);
        dc.FillRect(&rc, br);

        // 4. 绘制文字
        dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(clrCurrentText);
        
        HFONT hFont = GetFont();
        HFONT hOldFont = dc.SelectFont(hFont);

        TCHAR szText[MAX_PATH];
        GetWindowText(szText, MAX_PATH);
        dc.DrawText(szText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        dc.SelectFont(hOldFont);

        // 5. 绘制虚线焦点
        /*
        if ((state & ODS_FOCUS) && !bDisabled) {
            CRect rcFocus = rc;
            rcFocus.DeflateRect(3, 3);
            dc.DrawFocusRect(&rcFocus);
        }
        */
    }
};