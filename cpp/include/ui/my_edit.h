#pragma once

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlstr.h>
#include <atlcrack.h>

#include "ui/common.h"

class CMyEdit : public CWindowImpl<CMyEdit, CEdit> {
public:
    BEGIN_MSG_MAP(CMyEdit)
        MSG_WM_GETDLGCODE(OnGetDlgCode)
        MSG_WM_KEYDOWN(OnKeyDown)
    END_MSG_MAP()

    UINT OnGetDlgCode(LPMSG lpMsg) {
        // 允许基类处理（保留 Edit 的默认行为，如全选等）
        UINT uCode = DefWindowProc(); 

        // 如果检测到当前消息是键盘按键，并且按键是回车
        if (lpMsg != nullptr && lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_RETURN) {
            // 告诉对话框管理器：我需要处理回车键，不要拦截它
            uCode |= DLGC_WANTMESSAGE; 
        }
        return uCode;
    }

    void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
        if (nChar == VK_RETURN) {
            // 1. 捕获回车并将焦点还给父窗口, 自动触发EN_KILLFOCUS
            ::SetFocus(GetParent());
            // 2. 拦截消息, 防止回车产生系统提示音或触发默认按钮
            return; 
        }
        // 其他按钮交给原生 CEdit 处理
        SetMsgHandled(FALSE);
    }
};