#pragma once

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlstr.h>

#include "ui/common.h"

// ============================================================================
// 刻度文字定义
// ============================================================================
struct TrackLabel
{
    int value;         // 对应位置
    CString text;      // 显示文字
    COLORREF color = MY_COLOR_TRACKBAR_LABEL;  // 文字颜色
};

// ============================================================================
// 吸附 TrackBar
// ============================================================================
class CSnapTrackBar : public CWindowImpl<CSnapTrackBar, CTrackBarCtrl>
{
public:

    DECLARE_WND_SUPERCLASS(_T("SnapTrackBar"), CTrackBarCtrl::GetWndClassName())

    // =========================================================================
    // 配置
    // =========================================================================

    // 拖动吸附步长
    int m_snapStep = 10;

    // 刻度线步长
    int m_tickStep = 25;
    COLORREF m_tickColor = MY_COLOR_TRACKBAR_TICK;
    int m_tickWidth = 1;
    int m_tickHeight = 6;
    int m_tickSpacing = 4;
    int m_labelSpacing = 8;

    // 是否正在拖动
    bool m_dragging = false;

    // 自定义文字
    CSimpleArray<TrackLabel> m_labels;

public:

    BEGIN_MSG_MAP(CSnapTrackBar)
        MESSAGE_HANDLER(OCM_USER_HSCROLL, OnReflectedHScroll)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    END_MSG_MAP()

    // =========================================================================
    // 初始化
    // =========================================================================
    void Initialize(int minValue, int maxValue, int snapStep, int tickStep) {
        m_snapStep = snapStep;
        m_tickStep = tickStep;

        ModifyStyle(TBS_AUTOTICKS | TBS_VERT, TBS_NOTICKS | TBS_HORZ);

        SetRange(minValue, maxValue);
        SetTicFreq(tickStep);

        m_tickWidth = ScalePixelForWindow(m_hWnd, TRACKBAR_TICK_WIDTH);
        m_tickHeight = ScalePixelForWindow(m_hWnd, TRACKBAR_TICK_HEIGHT);
        m_tickSpacing = ScalePixelForWindow(m_hWnd, TRACKBAR_TICK_SPACING);
        m_labelSpacing = ScalePixelForWindow(m_hWnd, TRACKBAR_LABEL_SPACING);
    }

    int GetMinHeight() {
        // 获取滑块
        RECT rcThumb { 0, 0, 0, 0 };
        GetThumbRect(&rcThumb);

        // 当有label的时候才加入文字高度
        RECT rcLabelText { 0, 0, 0, 0 }; 
        if (m_labels.GetSize() > 0) {
            CDCHandle dc = GetDC();
            HFONT hFont = (HFONT)::SendMessage(GetParent(), WM_GETFONT, 0, 0);
            HFONT oldFont = (HFONT)SelectObject(dc, hFont);
            dc.DrawText(_T("100"), -1, &rcLabelText, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject(dc, oldFont);
        }

        return rcThumb.bottom + m_tickSpacing + m_tickHeight + m_labelSpacing + rcLabelText.bottom - rcLabelText.top;
    }

    // =========================================================================
    // 添加文字标签
    // =========================================================================
    void AddLabel(int value, const wchar_t* text, COLORREF color = MY_COLOR_TRACKBAR_LABEL) {
        TrackLabel lbl;
        lbl.value = value;
        lbl.text = text;
        lbl.color = color;

        m_labels.Add(lbl);

        Invalidate();
    }

    void RemoveAllLabel() {
        m_labels.RemoveAll();
        Invalidate();
    }

    // =========================================================================
    // 吸附计算
    // =========================================================================
    int SnapValue(int value) {
        if (m_snapStep <= 0)
            return value;

        int minVal = GetRangeMin();
        int maxVal = GetRangeMax();

        int relative = value - minVal;

        int snapped = ((relative + m_snapStep / 2) / m_snapStep) * m_snapStep;

        int retVal = minVal + snapped;
        if (retVal > maxVal) retVal = maxVal;

        return retVal;
    }

    LRESULT OnReflectedHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
        // 判断消息是否由我们的滑条发出
        if ((HWND)lParam == m_hWnd) {
            // 1. 获取滑条当前的真实位置
            int curPos = GetPos();

            // 2. 核心吸附算法：四舍五入到最近的 SNAP_STEP 倍数点位
            // 例如：若拖到 14，(14 + 5)/10 * 10 = 10；若拖到 16，(16 + 5)/10 * 10 = 20
            int snapPos = SnapValue(curPos);

            // 4. 如果计算出的吸附点位和当前滑条位置不一致，强制修正滑条位置
            if (curPos != snapPos) {
                SetPos(snapPos);
            }
        }
        return 0;
    }

    
    // =========================================================================
    // 防闪烁
    // =========================================================================
    LRESULT OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL&) {
        return 1;
    }

    // =========================================================================
    // 自绘
    // =========================================================================
    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
        PAINTSTRUCT ps;

        HDC hdc = BeginPaint(&ps);

        // 让原生 TrackBar 绘制
        DefWindowProc(WM_PAINT, (WPARAM)hdc, 0);

        int minVal = GetRangeMin();
        int maxVal = GetRangeMax();

        if (maxVal <= minVal) {
            EndPaint(&ps);
            return -1;
        }
        
        float rangeVal = (float)(maxVal - minVal);
        
        // 获取滑块
        RECT rcThumb {};
        GetThumbRect(&rcThumb);
        // 获取滑轨区域
        RECT rcChannel {};
        GetChannelRect(&rcChannel);

        int thumbWidth = rcThumb.right - rcThumb.left;
        int channelWidth = rcChannel.right - rcChannel.left;
        // 刻度起始位置位于滑轨起始加上一半的滑块宽度
        int thumbCenterStart = rcChannel.left + thumbWidth / 2;
        // 刻度整体宽度为滑块的中心点移动范围
        int thumbCenterMoveRange = channelWidth - thumbWidth;

        // 现在在原生控件上叠加绘制
        DrawCustomTicks(hdc, minVal, maxVal, rangeVal, thumbCenterStart, thumbCenterMoveRange, rcThumb);
        DrawLabels(hdc, minVal, maxVal, rangeVal, thumbCenterStart, thumbCenterMoveRange, rcThumb);

        EndPaint(&ps);

        return 0;
    }

    // =========================================================================
    // 绘制刻度线
    // =========================================================================
    void DrawCustomTicks(CDCHandle dc, int minVal, int maxVal, float rangeVal, int thumbCenterStart, int thumbCenterMoveRange, RECT rcThumb) {
        HPEN pen = CreatePen(PS_SOLID, m_tickWidth, m_tickColor);
        HPEN oldPen = (HPEN)SelectObject(dc, pen);

        for (int v = minVal; v <= maxVal; v += m_tickStep)
        {
            // 获取刻度的数值计算百分比
            float t = (float)(v - minVal) / rangeVal;
            // 计算刻度x坐标
            int x = thumbCenterStart + (int)(t * thumbCenterMoveRange);
            // 计算下方刻度y坐标
            int y1 = rcThumb.bottom + m_tickSpacing;
            int y2 = y1 + m_tickHeight;
            MoveToEx(dc, x, y1, nullptr);
            LineTo(dc, x, y2);
            // 计算上方刻度y坐标
            y1 = rcThumb.top - m_tickSpacing;
            y2 = y1 - m_tickHeight;
            MoveToEx(dc, x, y1, nullptr);
            LineTo(dc, x, y2);
        }

        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }

    // =========================================================================
    // 绘制文字
    // =========================================================================
    void DrawLabels(CDCHandle dc, int minVal, int maxVal, float rangeVal, int thumbCenterStart, int thumbCenterMoveRange, RECT rcThumb) {
        // 获取父窗口字体
        HFONT hFont = (HFONT)::SendMessage(GetParent(), WM_GETFONT, 0, 0);
        HFONT oldFont = (HFONT)SelectObject(dc, hFont);

        SetBkMode(dc, TRANSPARENT);

        RECT hwndRect {};
        if (!GetWindowRect(&hwndRect)) return;
        
        int count = m_labels.GetSize();
        for (int i = 0; i < count; i++) {
            float t = (float)(m_labels[i].value - minVal) / rangeVal;

            int x = thumbCenterStart + (int)(t * thumbCenterMoveRange);

            // 获取文字对应的尺寸
            RECT rcLabelText = { 0, 0, 0, 0 }; 
            // DT_CALCRECT: 只计算不绘图
            // DT_SINGLELINE: 强制单行，忽略换行符
            // DT_NOPREFIX: 如果文字里有 '&'，不把它当成下划线快捷键计算
            dc.DrawText(m_labels[i].text, -1, &rcLabelText, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);

            int x_min = x - (rcLabelText.right - rcLabelText.left) / 2;
            int x_max = x_min + (rcLabelText.right - rcLabelText.left);
            
            if (x_max > (hwndRect.right - hwndRect.left)) {
                x_min -= x_max - (hwndRect.right - hwndRect.left);
            }

            if (x_min < 0) {
                x_min = 0;
            }

            SetTextColor(dc, m_labels[i].color);
            TextOutW(
                dc,
                x_min,
                rcThumb.bottom + m_tickSpacing + m_tickHeight + m_labelSpacing,
                m_labels[i].text,
                m_labels[i].text.GetLength()
            );
        }

        SelectObject(dc, oldFont);
    }

};
