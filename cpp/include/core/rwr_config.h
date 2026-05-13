#pragma once
#include <xmllite.h>
#include <atlbase.h>
#include <atlstr.h>
#include <atlapp.h>
#include <atlcoll.h>

#pragma comment(lib, "xmllite.lib")

class RWRConfigManager {
public:
    CSimpleMap<CString, CString> m_configs;

    bool LoadConfig(LPCTSTR filePath) {
        CComPtr<IStream> pFileStream;
        if (FAILED(SHCreateStreamOnFileW(filePath, STGM_READ, &pFileStream))) 
            return false;

        CComPtr<IXmlReader> pReader;
        if (FAILED(CreateXmlReader(__uuidof(IXmlReader), (void**)&pReader, NULL)))
            return false;

        if (FAILED(pReader->SetInput(pFileStream)))
            return false;

        XmlNodeType nodeType;
        CString strCurrentTag;

        while (S_OK == pReader->Read(&nodeType)) {
            switch (nodeType) {
            case XmlNodeType_Element: {
                LPCTSTR pwszLocalName = nullptr;
                pReader->GetLocalName(&pwszLocalName, NULL);
                strCurrentTag = pwszLocalName;

                // 处理带属性的标签，如 <fullscreen value="0"/>
                if (S_OK == pReader->MoveToAttributeByName(L"value", NULL)) {
                    LPCTSTR pwszValue = nullptr;
                    pReader->GetValue(&pwszValue, NULL);
                    m_configs.Add(strCurrentTag, pwszValue);
                }

                // 如果是空标签（如 <parameters/>），清空标记
                if (pReader->IsEmptyElement()) {
                    strCurrentTag.Empty();
                }
                break;
            }

            case XmlNodeType_Text:
            case XmlNodeType_CDATA: {
                // 处理文本内容或 CDATA 内容
                LPCTSTR pwszValue = nullptr;
                if (S_OK == pReader->GetValue(&pwszValue, NULL)) {
                    if (!strCurrentTag.IsEmpty()) {
                        m_configs.Add(strCurrentTag, pwszValue);
                    }
                }
                break;
            }

            case XmlNodeType_EndElement:
                strCurrentTag.Empty();
                break;
            }
        }
        return true;
    }

    // 辅助函数：快速获取配置
    CString GetValue(LPCTSTR key) {
        // 1. 先查找 Key 所在的索引 (返回 -1 表示没找到)
        int nIndex = m_configs.FindKey(key);
        
        // 2. 如果找到了，根据索引取 Value
        if (nIndex != -1) {
            return m_configs.GetValueAt(nIndex);
        }
        
        // 3. 没找到则返回空字符串
        return _T("");
    }
};

/*
RWRConfigManager config;
if (config.LoadConfig(L"config.xml")) {
    CString renderdevice = config.GetValue(L"renderdevice"); // 获取 CDATA 内容
    CString fullscreen = config.GetValue(L"fullscreen"); // 获取属性内容
    CString lang = config.GetValue(L"language"); // 获取普通文本
}
*/