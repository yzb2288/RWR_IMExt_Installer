#pragma once

#include <windows.h>

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlstr.h>
#include <atlcoll.h>

#include "cjson/cJSON.h"

#include "i18n/i18n.h"
#include "utils/file_utils.h"
#include "ui/common.h"

// 树节点数据类
class FileTreeNodeData;
typedef CSimpleMap<CString, FileTreeNodeData*> SubNodeMap;

class FileTreeNodeData {
public:
    COLORREF    color;
    bool        open;
    bool        loaded;
    CString*    pRelFilePath;
    CString*    pAdditionalText;
    SubNodeMap* pSubNodes;

public:
    // 1. 标准构造函数
    FileTreeNodeData() : 
        color(0), open(false), loaded(false), 
        pRelFilePath(nullptr), pAdditionalText(nullptr), pSubNodes(nullptr) {}

    // 2. 深拷贝构造函数
    FileTreeNodeData(const FileTreeNodeData& other) {
        InternalCopy(other);
    }

    // 3. 复制运算符重载
    FileTreeNodeData& operator=(const FileTreeNodeData& other) {
        if (this != &other) {
            InternalClearAll();
            InternalCopy(other);
        }
        return *this;
    }

    ~FileTreeNodeData() {
        InternalClearAll();
    }

    // --- 功能接口 ---

    // A. 下标访问
    FileTreeNodeData* operator[](LPCTSTR szName) const {
        return FindSubNode(szName);
    }

    FileTreeNodeData* operator[](int index) const {
        return GetSubNodeAt(index);
    }

    // B. 获取长度
    int GetSubNodeCount() const {
        return pSubNodes ? pSubNodes->GetSize() : 0;
    }

    // C. 索引获取节点名称
    CString GetSubNodeNameAt(int index) const {
        if (pSubNodes && index >= 0 && index < pSubNodes->GetSize()) {
            return pSubNodes->GetKeyAt(index);
        }
        return CString(_T(""));
    }

    // D. 索引获取子节点指针
    FileTreeNodeData* GetSubNodeAt(int index) const {
        if (pSubNodes && index >= 0 && index < pSubNodes->GetSize()) {
            return pSubNodes->GetValueAt(index);
        }
        return nullptr;
    }

    FileTreeNodeData* FindSubNode(LPCTSTR szName) const {
        if (!pSubNodes) return nullptr;
        return pSubNodes->Lookup(szName);
    }

    // --- 链式调用功能接口 ---
    /*
    FileTreeNodeData* root = new FileTreeNodeData();
    root->SetColor(RGB(255,0,0))
        .AddSubNode(_T("Node1"), &(new FileTreeNodeData())->SetOpen(true))
        .AddSubNode(_T("Node2"), &(new FileTreeNodeData())->SetOpen(false))
        .AddSubNode(_T("SubNodeA"), new FileTreeNodeData())
    );
    */
    FileTreeNodeData& AddSubNode(const CString& name, FileTreeNodeData* pNode) {
        if (!pSubNodes) pSubNodes = new SubNodeMap();
        pSubNodes->Add(name, pNode);
        return *this;
    }

    FileTreeNodeData& SetColor(DWORD c) { this->color = c; return *this; }
    FileTreeNodeData& SetOpen(bool o)   { this->open = o; return *this; }
    FileTreeNodeData& SetLoaded(bool l)   { this->loaded = l; return *this; }

    FileTreeNodeData& SetRelFilePath(LPCTSTR szSrc) {
        // 1. 先清理旧内存
        if (pRelFilePath) {
            delete pRelFilePath;
            pRelFilePath = nullptr;
        }
        // 2. 如果源不为 nullptr，则创建新副本
        if (szSrc) {
            pRelFilePath = new CString(szSrc);
        }
        return *this;
    }

    FileTreeNodeData& SetAdditionalText(LPCTSTR szSrc) {
        // 1. 先清理旧内存
        if (pAdditionalText) {
            delete pAdditionalText;
            pAdditionalText = nullptr;
        }
        // 2. 如果源不为 nullptr，则创建新副本
        if (szSrc) {
            pAdditionalText = new CString(szSrc);
        }
        return *this;
    }

    FileTreeNodeData& ClearSubNodes() {
        InternalClearSubNodes();
        return *this;
    }

    FileTreeNodeData& ClearAll() {
        InternalClearAll();
        color = 0;
        open = false;
        loaded = false;
        return *this;
    }

private:
    // 内部清理逻辑
    void InternalClearAll() {
        if (pRelFilePath) { delete pRelFilePath; pRelFilePath = nullptr; }
        if (pAdditionalText) { delete pAdditionalText; pAdditionalText = nullptr; }
        
        InternalClearSubNodes();
    }

    void InternalClearSubNodes() {
        if (pSubNodes) {
            // 递归释放子节点内存
            for (int i = 0; i < pSubNodes->GetSize(); i++) {
                delete pSubNodes->GetValueAt(i);
            }
            delete pSubNodes;
            pSubNodes = nullptr;
        }
    }

    // 内部深拷贝逻辑
    void InternalCopy(const FileTreeNodeData& other) {
        this->color = other.color;
        this->open = other.open;
        this->loaded = other.loaded;

        // 拷贝指针内容
        this->pRelFilePath = other.pRelFilePath ? new CString(*(other.pRelFilePath)) : nullptr;
        this->pAdditionalText = other.pAdditionalText ? new CString(*(other.pAdditionalText)) : nullptr;

        // 递归拷贝子节点
        if (other.pSubNodes) {
            this->pSubNodes = new SubNodeMap();
            for (int i = 0; i < other.pSubNodes->GetSize(); i++) {
                CString name = other.pSubNodes->GetKeyAt(i);
                FileTreeNodeData* pOriginal = other.pSubNodes->GetValueAt(i);
                if (pOriginal) {
                    // 这里会触发递归调用拷贝构造函数
                    this->pSubNodes->Add(name, new FileTreeNodeData(*pOriginal));
                }
            }
        } else {
            this->pSubNodes = nullptr;
        }
    }
};


class CTreeRedrawLock {
    CTreeViewCtrl& m_tree;
    int& m_refCount;

public:
    // 构造时：增加计数，如果从 0 变 1 则锁定重绘
    CTreeRedrawLock(CTreeViewCtrl& tree, int& refCount) 
        : m_tree(tree), m_refCount(refCount) 
    {
        if (m_refCount == 0) {
            m_tree.SetRedraw(FALSE);
        }
        m_refCount++;
    }

    // 析构时：减少计数，如果归 0 则恢复重绘并刷新
    ~CTreeRedrawLock() {
        m_refCount--;
        if (m_refCount == 0) {
            m_tree.SetRedraw(TRUE);
            m_tree.Invalidate();
        }
    }
};

template <class T>
class FileTreeView
{
private:
    UINT m_treeId;
    UINT m_updateTreeMsg;
    int m_treeRedrawLockCount = 0;

    CString m_rwrInstallPath;
    CString m_imextPath;

public:

    BEGIN_MSG_MAP(FileTreeView<T>)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog) // 兼顾对话框
        MESSAGE_HANDLER(m_updateTreeMsg, OnUpdateTree)
        NOTIFY_HANDLER(m_treeId, TVN_ITEMEXPANDING, OnTreeExpanding)
        NOTIFY_HANDLER(m_treeId, NM_CUSTOMDRAW, OnTreeCustomDraw)
        NOTIFY_HANDLER(m_treeId, NM_RCLICK, OnTreeRightClick)
    END_MSG_MAP()

    FileTreeView(UINT treeId, UINT updateTreeMsg)
    : m_treeId(treeId),
      m_updateTreeMsg(updateTreeMsg),
      m_treeNodeDataRoot()
    {
        InitTreeNodeDataRoot();
    }
    
    ~FileTreeView() {
    }

protected:
    CTreeViewCtrl m_tree;
    FileTreeNodeData m_treeNodeDataRoot;

    void InitTreeNodeDataRoot() {
        m_treeNodeDataRoot
            .ClearAll()
            .SetOpen(true)
            .SetLoaded(false);
    }

    void SetRwrInstallPath(CString& rwrInstallPath) {
        m_rwrInstallPath = rwrInstallPath;
    }

    void SetImextPath(CString& imextPath) {
        m_imextPath = imextPath;
    }

    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
        InitTreeView();
        bHandled = FALSE; // 重要：让出控制权，使派生类/其他基类能继续处理 WM_CREATE
        return 0;
    }

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
        InitTreeView();
        bHandled = FALSE; // 让对话框逻辑继续
        return 0;
    }

    void InitTreeView() {
        T* pT = static_cast<T*>(this);
        // 如果是普通窗口，可以在这里创建控件
        // 如果是对话框，假设 ID 为 IDC_TREE1
        if (pT->m_hWnd) {
            m_tree = pT->GetDlgItem(m_treeId); 
            if (!m_tree.m_hWnd) {
                // 如果找不到 ID，说明是普通窗口，动态创建
                m_tree.Create(pT->m_hWnd, pT->rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
                TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT, 0, m_treeId);
            } else {
                m_tree.ModifyStyle(0, WS_CHILD | WS_VISIBLE | WS_BORDER |
                TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT);
            }
        }
    }

    LRESULT OnUpdateTree(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
        CTreeRedrawLock lock(m_tree, m_treeRedrawLockCount);

        m_tree.DeleteAllItems();
        
        if (m_treeNodeDataRoot.pSubNodes)
        {
            LoadSubNodes(TVI_ROOT, m_treeNodeDataRoot.pSubNodes);
        }
        
        m_treeNodeDataRoot
            .SetOpen(true)
            .SetLoaded(true);

        bHandled = FALSE;
        return 0;
    }

    void LoadSubNodes(HTREEITEM parent, SubNodeMap* pParentSubNodes) {
        if (!pParentSubNodes) {
            return;
        }
        
        for (int i = 0; i < pParentSubNodes->GetSize(); i++) {
            CString nodeName = pParentSubNodes->GetKeyAt(i);
            FileTreeNodeData* nodeData = pParentSubNodes->GetValueAt(i);
            if (nodeData) {
                HTREEITEM node = m_tree.InsertItem(GetNodeText(nodeName, nodeData), parent, TVI_LAST);
                m_tree.SetItemData(node, (DWORD_PTR)nodeData);

                if (nodeData->pSubNodes) {
                    // 懒加载直接给有子节点的新增占位节点
                    m_tree.InsertItem(_T(""), node, TVI_LAST);
                    if (nodeData->open) {
                        // 发送展开消息给需要open的递归执行
                        m_tree.Expand(node, TVE_EXPAND);
                    }
                } else {
                    // 没有子节点的新节点为了数据统一在这里设置open和loaded状态
                    nodeData->SetOpen(false);
                    nodeData->SetLoaded(true);
                }
            }
        }
    }

    CString GetNodeText(const CString& nodeName, FileTreeNodeData* nodeData)
    {
        CString nodeText(nodeName);
        if (nodeData && nodeData->pAdditionalText && nodeData->pAdditionalText->GetLength() > 0)
        {
            nodeText += CString(_T(" - ")) + *nodeData->pAdditionalText;
        }
        return nodeText;
    }

    LRESULT OnTreeExpanding(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
        LPNMTREEVIEW pnmTV = (LPNMTREEVIEW)(pnmh);
    
        // 只在用户执行“展开”操作时触发
        if (pnmTV->action == TVE_EXPAND)
        {
            return ExpandNode(pnmTV->itemNew.hItem);
        }
        if (pnmTV->action == TVE_COLLAPSE)
        {
            return CollapseNode(pnmTV->itemNew.hItem);
        }
        return 0; // 返回 0 允许继续展开
    }

    LRESULT ExpandNode(HTREEITEM node) {
        // RAII 重绘锁
        CTreeRedrawLock lock(m_tree, m_treeRedrawLockCount);

        FileTreeNodeData* nodeData = (FileTreeNodeData*)m_tree.GetItemData(node);

        if (!nodeData) {
            return 1;
        }

        if (nodeData->loaded)
        {
            nodeData->SetOpen(true);
            return 0;
        }

        // 清空其下所有子节点
        HTREEITEM child = m_tree.GetChildItem(node);
        while (child != NULL) {
            m_tree.DeleteItem(child);
            child = m_tree.GetChildItem(node);
        }

        LoadSubNodes(node, nodeData->pSubNodes);
        
        nodeData->SetOpen(true);
        nodeData->SetLoaded(true);

        return 0;
    }

    LRESULT CollapseNode(HTREEITEM node) {
        FileTreeNodeData* nodeData = (FileTreeNodeData*)m_tree.GetItemData(node);

        if (!nodeData) {
            return 1;
        }

        nodeData->SetOpen(false);

        return 0;
    }

    LRESULT OnTreeCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
        LPNMTVCUSTOMDRAW pCustomDraw = (LPNMTVCUSTOMDRAW)(pnmh);

        switch (pCustomDraw->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:
                // 告诉系统: 在绘制每个物品(Item)之前通知我
                return CDRF_NOTIFYITEMDRAW;

            case CDDS_ITEMPREPAINT: {
                FileTreeNodeData* nodeData = (FileTreeNodeData*)(pCustomDraw->nmcd.lItemlParam);
                bool bHasHighlightBackground = (pCustomDraw->clrTextBk == GetSysColor(COLOR_HIGHLIGHT));

                if (!bHasHighlightBackground) {
                    // 没有高亮底色的情况下才显示自定义颜色
                    if (nodeData) {
                        // 文字颜色
                        pCustomDraw->clrText = nodeData->color;
                        // 背景色
                        // pCustomDraw->clrTextBk = RGB(240, 240, 240);
                    }
                }

                return CDRF_DODEFAULT;
            }
        }

        return CDRF_DODEFAULT;
    }

    LRESULT OnTreeRightClick(int, LPNMHDR, BOOL&) {
        DWORD pos = GetMessagePos();
        POINT pt = { GET_X_LPARAM(pos), GET_Y_LPARAM(pos) };

        POINT ptClient = pt;
        m_tree.ScreenToClient(&ptClient);

        TVHITTESTINFO hit = { 0 };
        hit.pt = ptClient;

        HTREEITEM item = m_tree.HitTest(&hit);
        if (!item) {return 0;}

        m_tree.SelectItem(item);

        FileTreeNodeData* nodeData = (FileTreeNodeData*)m_tree.GetItemData(item);
        if (!nodeData) {
            return 0;
        }

        CMenu menu;
        menu.CreatePopupMenu();

        if (nodeData->pSubNodes) {
            menu.AppendMenu(MF_STRING, IDM_EXPAND_ALL, _TR(IDS_EXPAND_ALL));
            menu.AppendMenu(MF_STRING, IDM_COLLAPSE_ALL, _TR(IDS_COLLAPSE_ALL));
        }
        
        //改成相对路径
        CString gameFilePath;
        CString imextFilePath;
        if (nodeData->pRelFilePath)
        {
            gameFilePath = FileUtils::NormalizePath(m_rwrInstallPath + _T("\\") + *nodeData->pRelFilePath);
            imextFilePath = FileUtils::NormalizePath(m_imextPath + _T("\\") + *nodeData->pRelFilePath);

            bool gameFileExists = FileUtils::FileExists(gameFilePath);
            bool imextFileExists = FileUtils::FileExists(imextFilePath);

            
            if (gameFileExists) {menu.AppendMenu(MF_STRING, IDM_OPEN_IN_RWR, _TR(IDS_OPEN_IN_RWR));}
            if (imextFileExists) {menu.AppendMenu(MF_STRING, IDM_OPEN_IN_IMEXT, _TR(IDS_OPEN_IN_IMEXT));}
            if (gameFileExists) {menu.AppendMenu(MF_STRING, IDM_SHOW_IN_RWR, _TR(IDS_SHOW_IN_RWR));}
            if (imextFileExists) {menu.AppendMenu(MF_STRING, IDM_SHOW_IN_IMEXT, _TR(IDS_SHOW_IN_IMEXT));}

            if (nodeData->pSubNodes && (gameFileExists || imextFileExists))
            {
                menu.InsertMenuW(2, MF_BYPOSITION | MF_SEPARATOR);
            }
        }

        T* pThis = static_cast<T*>(this);
        int cmd = menu.TrackPopupMenu(TPM_RETURNCMD, pt.x, pt.y, pThis->m_hWnd);

        HandleMenu(cmd, item, nodeData, gameFilePath, imextFilePath);

        return 0;
    }

    void HandleMenu(int cmd, HTREEITEM node, FileTreeNodeData* nodeData, const CString& gameFilePath, const CString& imextFilePath) {
        if (!nodeData) return;

        switch (cmd)
        {
        case IDM_EXPAND_ALL:
        {
            SetNodeDataOpenRecursively(nodeData, true);
            ExpandNode(node); // 此处不会通知OnTreeExpanding
            m_tree.Expand(node, TVE_EXPAND);
            break;
        }
        case IDM_COLLAPSE_ALL:
        {
            SetNodeDataOpenRecursively(nodeData, false);
            m_tree.Expand(node, TVE_COLLAPSE);
            break;
        }
        case IDM_OPEN_IN_RWR:
        {
            FileUtils::OpenFile(gameFilePath);
            break;
        }
        case IDM_OPEN_IN_IMEXT:
        {
            FileUtils::OpenFile(imextFilePath);
            break;
        }
        case IDM_SHOW_IN_RWR:
        {
            FileUtils::ShowInFolder(gameFilePath);
            break;
        }
        case IDM_SHOW_IN_IMEXT:
        {
            FileUtils::ShowInFolder(imextFilePath);
            break;
        }
        }
    }

    void SetNodeDataOpenRecursively(FileTreeNodeData* nodeData, bool setOpen) {
        // 关闭一次后，不能再全部打开的问题
        if (!nodeData) {
            return;
        }

        // 无论是否有子节点都设置open状态
        nodeData->SetOpen(setOpen);
        // 需要重新取消loaded状态才能刷新
        nodeData->SetLoaded(false);

        // 递归设置状态
        if (nodeData->pSubNodes) {
            for (int i = 0; i < nodeData->pSubNodes->GetSize(); i++) {
                FileTreeNodeData* subNodeData = nodeData->pSubNodes->GetValueAt(i);
                SetNodeDataOpenRecursively(subNodeData, setOpen);
            }
        }
    }

    void insertFileRelPathNode(
        FileTreeNodeData* parentNodeData,
        const cJSON* const fileRelPathInfo,
        COLORREF color = RGB(0, 0, 0),
        LPCTSTR overwriteAdditionalText = nullptr,
        int openLevel = 0
    ) {
        if (!fileRelPathInfo) return;
        cJSON* child = fileRelPathInfo->child;
        while (child)
        {
            CString fileRelPathFirst(L""); // 需要验证FileUtils::SplitPathFirstComponent返回false情况
            CString fileRelPathRest(CA2W(child->string, CP_UTF8));

            FileTreeNodeData* currentNodeData = parentNodeData;
            if (fileRelPathRest.IsEmpty())
            {
                // 当最开始的文件路径就是空时, 加一个空的节点
                if (!(*currentNodeData)[fileRelPathFirst]) {
                    currentNodeData->AddSubNode(fileRelPathFirst,
                        &(new FileTreeNodeData())->SetColor(color)
                        .SetAdditionalText(overwriteAdditionalText ? overwriteAdditionalText : CA2W(child->valuestring, CP_UTF8)) // 需要加翻译
                        .SetRelFilePath(fileRelPathFirst));
                }
            } else {
                int openLevelCount = openLevel;
                CString newRelFilePath(L"");
                while (!fileRelPathRest.IsEmpty()) {
                    if (openLevelCount > 0) {
                        currentNodeData->SetOpen(true);
                        openLevelCount--;
                    }

                    FileUtils::SplitPathFirstComponent(fileRelPathRest, fileRelPathFirst, fileRelPathRest);

                    if (newRelFilePath.IsEmpty()) {
                        newRelFilePath += fileRelPathFirst;
                    } else {
                        newRelFilePath += CString(L"\\") + fileRelPathFirst;
                    }

                    // 加节点
                    FileTreeNodeData* newSubNodeData = (*currentNodeData)[fileRelPathFirst];
                    if (!newSubNodeData) {
                        newSubNodeData = new FileTreeNodeData();
                        newSubNodeData->SetColor(color)
                            .SetAdditionalText(fileRelPathRest.IsEmpty() ? (overwriteAdditionalText ? overwriteAdditionalText : CA2W(child->valuestring, CP_UTF8)) : nullptr) // 需要加翻译
                            .SetRelFilePath(
                                newRelFilePath
                            );
                        
                        currentNodeData->AddSubNode(fileRelPathFirst, newSubNodeData);
                    }

                    currentNodeData = newSubNodeData;
                }
            }
            
            child = child->next;
        }
    }

    void insertFileRelPathNode(
        FileTreeNodeData* parentNodeData,
        const CSimpleArray<CString>* const fileRelPathList,
        COLORREF color = RGB(0, 0, 0),
        LPCTSTR overwriteAdditionalText = nullptr,
        int openLevel = 0
    ) {
        if (!fileRelPathList) return;
        int count = fileRelPathList->GetSize();
        if (count == 0) return;
        for (int i = 0; i < count; i++) {

            CString fileRelPathFirst(L""); // 需要验证FileUtils::SplitPathFirstComponent返回false情况
            CString fileRelPathRest((*fileRelPathList)[i]);

            FileTreeNodeData* currentNodeData = parentNodeData;
            if (fileRelPathRest.IsEmpty())
            {
                // 当最开始的文件路径就是空时, 加一个空的节点
                if (!(*currentNodeData)[fileRelPathFirst]) {
                    currentNodeData->AddSubNode(fileRelPathFirst,
                        &(new FileTreeNodeData())->SetColor(color)
                        .SetAdditionalText(overwriteAdditionalText ? overwriteAdditionalText : nullptr) // 需要加翻译
                        .SetRelFilePath(fileRelPathFirst));
                }
            } else {
                int openLevelCount = openLevel;
                CString newRelFilePath(L"");
                while (!fileRelPathRest.IsEmpty()) {
                    if (openLevelCount > 0) {
                        currentNodeData->SetOpen(true);
                        openLevelCount--;
                    }

                    FileUtils::SplitPathFirstComponent(fileRelPathRest, fileRelPathFirst, fileRelPathRest);

                    if (newRelFilePath.IsEmpty()) {
                        newRelFilePath += fileRelPathFirst;
                    } else {
                        newRelFilePath += CString(L"\\") + fileRelPathFirst;
                    }

                    // 加节点
                    FileTreeNodeData* newSubNodeData = (*currentNodeData)[fileRelPathFirst];
                    if (!newSubNodeData) {
                        newSubNodeData = new FileTreeNodeData();
                        newSubNodeData->SetColor(color)
                            .SetAdditionalText(fileRelPathRest.IsEmpty() ? (overwriteAdditionalText ? overwriteAdditionalText : nullptr) : nullptr) // 需要加翻译
                            .SetRelFilePath(
                                newRelFilePath
                            );
                        
                        currentNodeData->AddSubNode(fileRelPathFirst, newSubNodeData);
                    }

                    currentNodeData = newSubNodeData;
                }
            }
        }
    }
};