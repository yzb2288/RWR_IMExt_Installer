#pragma once
#include <windows.h>
#include <oleauto.h>
#include <atlbase.h>
#include <atlcomcli.h>
#include <atlstr.h>
#include <atlsimpcoll.h>
#include <msxml2.h>
#include <msxml6.h>

#pragma comment(lib, "msxml6.lib")

#include "utils/file_utils.h"
#include "utils/string_utils.h"

namespace XmlUtils {

    // ==================== 读取分辨率（MSXML 6.0） ====================
    // 使用 MSXML 6.0 解析 Running with rifles 游戏配置 config.xml 中
    // <videomode> 标签内的分辨率宽高。
    // 默认路径为 "%APPDATA%\Running with rifles\config.xml"
    // （即 C:\Users\当前Windows用户名\AppData\Roaming\Running with rifles\config.xml），
    // 也可通过 configFilePath 参数传入自定义路径（例如用示例配置测试）。
    // 兼容 DX9 模式内容："<![CDATA[2560 x 1440 @ 32-bit colour]]>"
    // 兼容 OpenGL 模式内容："<![CDATA[2560 x 1440]]>"
    // 字符串分割复用 utils/string_utils.h 中的 StringUtils::Split。
    // 成功返回 true，outWidth / outHeight 输出分辨率宽高；失败返回 false。
    inline bool ReadConfigVideoModeResolution(LPCTSTR configFilePath, int& outWidth, int& outHeight) {
        if (configFilePath == NULL || *configFilePath == _T('\0') || !FileUtils::FileExists(configFilePath)) {
            return false;
        }

        outWidth = 0;
        outHeight = 0;

        // 2. 确保当前线程已初始化 COM（重复初始化只增加计数，返回 S_FALSE）
        HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            return false;
        }
        const bool bNeedUninit = SUCCEEDED(hr);

        bool bResult = false;
        CComPtr<IXMLDOMDocument2> pDoc;
        hr = pDoc.CoCreateInstance(CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER);
        do {
            if (FAILED(hr) || pDoc == NULL) {
                break;
            }

            // 3. 同步加载，保留空白，不做 DTD 校验与外部实体解析
            pDoc->put_async(VARIANT_FALSE);
            pDoc->put_preserveWhiteSpace(VARIANT_TRUE);
            pDoc->put_validateOnParse(VARIANT_FALSE);
            pDoc->put_resolveExternals(VARIANT_FALSE);

            CComBSTR bstrPath(configFilePath);
            CComVariant varPath(bstrPath);
            VARIANT_BOOL bLoaded = VARIANT_FALSE;
            hr = pDoc->load(varPath, &bLoaded);
            if (FAILED(hr) || bLoaded != VARIANT_TRUE) {
                break;
            }

            // 4. 查找 <videomode> 节点（取文档中第一个）
            CComPtr<IXMLDOMNodeList> pNodeList;
            hr = pDoc->getElementsByTagName(CComBSTR(L"videomode"), &pNodeList);
            if (FAILED(hr) || pNodeList == NULL) {
                break;
            }
            long nNodeCount = 0;
            if (FAILED(pNodeList->get_length(&nNodeCount)) || nNodeCount <= 0) {
                break;
            }

            CComPtr<IXMLDOMNode> pVideoModeNode;
            hr = pNodeList->get_item(0, &pVideoModeNode);
            if (FAILED(hr) || pVideoModeNode == NULL) {
                break;
            }

            // 5. 取节点文本（CDATA 内容会一并包含），去掉首尾空白
            CComBSTR bstrModeText;
            hr = pVideoModeNode->get_text(&bstrModeText);
            if (FAILED(hr)) {
                break;
            }

            CString modeText(bstrModeText);
            modeText.Trim();

            // 6. 使用 StringUtils::Split 按空白字符分割
            //    DX9:    "2560 x 1440 @ 32-bit colour" -> { "2560", "x", "1440", "@", "32-bit", "colour" }
            //    OpenGL: "2560 x 1440"                 -> { "2560", "x", "1440" }
            CSimpleArray<CString> tokens = StringUtils::Split(modeText);
            if (tokens.GetSize() < 3) {
                break;
            }
            if (tokens[1].CompareNoCase(_T("x")) != 0) {
                break;
            }

            outWidth = _ttoi(tokens[0]);
            outHeight = _ttoi(tokens[2]);
            bResult = (outWidth > 0 && outHeight > 0);
        } while (false);

        if (bNeedUninit) {
            ::CoUninitialize();
        }
        return bResult;
    }

    // ==================== 写入分辨率（MSXML 6.0） ====================
    // 使用 MSXML 6.0 将 config.xml 中 <videomode> 标签内的分辨率修改为指定宽高。
    // configFilePath 为 config.xml 的完整路径（必填）。
    // 修改方式：优先定位节点内的 CDATA 子节点并替换其 data 中的
    // "旧宽 x 旧高" 小段（保留 <![CDATA[ ... ]]> 包装与 " @ 32-bit colour" 等后缀），
    // 没有 CDATA 时退而修改第一个非空白文本节点；节点为空则新建文本节点。
    // 其他节点内容不变；保存由 MSXML 写回原文件（保留原编码、不添加 BOM）。
    // 成功返回 true；路径为空、宽高非法、解析失败或保存失败返回 false。
    inline bool WriteConfigVideoModeResolution(LPCTSTR configFilePath, int width, int height) {
        if (configFilePath == NULL || *configFilePath == _T('\0') || !FileUtils::FileExists(configFilePath) || width <= 0 || height <= 0) {
            return false;
        }

        HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            return false;
        }
        const bool bNeedUninit = SUCCEEDED(hr);

        bool bResult = false;
        CComPtr<IXMLDOMDocument2> pDoc;
        hr = pDoc.CoCreateInstance(CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER);
        do {
            if (FAILED(hr) || pDoc == NULL) {
                break;
            }

            pDoc->put_async(VARIANT_FALSE);
            pDoc->put_preserveWhiteSpace(VARIANT_TRUE);
            pDoc->put_validateOnParse(VARIANT_FALSE);
            pDoc->put_resolveExternals(VARIANT_FALSE);

            CComBSTR bstrPath(configFilePath);
            CComVariant varPath(bstrPath);
            VARIANT_BOOL bLoaded = VARIANT_FALSE;
            hr = pDoc->load(varPath, &bLoaded);
            if (FAILED(hr) || bLoaded != VARIANT_TRUE) {
                break;
            }

            // 1. 查找 <videomode> 节点（取文档中第一个）
            CComPtr<IXMLDOMNodeList> pNodeList;
            hr = pDoc->getElementsByTagName(CComBSTR(L"videomode"), &pNodeList);
            if (FAILED(hr) || pNodeList == NULL) {
                break;
            }
            long nNodeCount = 0;
            if (FAILED(pNodeList->get_length(&nNodeCount)) || nNodeCount <= 0) {
                break;
            }

            CComPtr<IXMLDOMNode> pVideoModeNode;
            hr = pNodeList->get_item(0, &pVideoModeNode);
            if (FAILED(hr) || pVideoModeNode == NULL) {
                break;
            }

            // 2. 在子节点中定位内容载体：优先 CDATA 节点，其次第一个非空白文本节点
            CComPtr<IXMLDOMNodeList> pChildList;
            hr = pVideoModeNode->get_childNodes(&pChildList);
            if (FAILED(hr) || pChildList == NULL) {
                break;
            }
            long nChildCount = 0;
            pChildList->get_length(&nChildCount);

            CComPtr<IXMLDOMCharacterData> pTargetData;
            CComPtr<IXMLDOMCharacterData> pFallbackText;
            for (long i = 0; i < nChildCount; ++i) {
                CComPtr<IXMLDOMNode> pChild;
                if (FAILED(pChildList->get_item(i, &pChild)) || pChild == NULL) {
                    continue;
                }

                DOMNodeType nodeType = NODE_INVALID;
                pChild->get_nodeType(&nodeType);
                if (nodeType == NODE_CDATA_SECTION) {
                    CComQIPtr<IXMLDOMCharacterData> pCData(pChild);
                    if (pCData != NULL) {
                        pTargetData = pCData;
                        break;
                    }
                } else if (nodeType == NODE_TEXT && pFallbackText == NULL) {
                    CComQIPtr<IXMLDOMCharacterData> pText(pChild);
                    if (pText != NULL) {
                        CComBSTR bstrTextData;
                        if (SUCCEEDED(pText->get_data(&bstrTextData))) {
                            CString strTextData(bstrTextData);
                            strTextData.Trim();
                            if (!strTextData.IsEmpty()) {
                                pFallbackText = pText;
                            }
                        }
                    }
                }
            }
            if (pTargetData == NULL) {
                pTargetData = pFallbackText;
            }

            CString newResolution;
            newResolution.Format(_T("%d x %d"), width, height);

            if (pTargetData != NULL) {
                // 3a. 取旧内容，只替换 "旧宽 x 旧高" 这一段，保留 CDATA 与后缀（如 " @ 32-bit colour"）
                CComBSTR bstrOldData;
                if (FAILED(pTargetData->get_data(&bstrOldData))) {
                    break;
                }

                CString innerText(bstrOldData);
                CString innerTextTrimmed = innerText;
                innerTextTrimmed.Trim();
                CSimpleArray<CString> tokens = StringUtils::Split(innerTextTrimmed);
                if (tokens.GetSize() < 3) {
                    break;
                }
                if (tokens[1].CompareNoCase(_T("x")) != 0) {
                    break;
                }

                // 宽高必须为纯数字，防止误伤其它格式内容
                auto isAllDigits = [](const CString& text) -> bool {
                    for (int i = 0; i < text.GetLength(); ++i) {
                        if (text[i] < _T('0') || text[i] > _T('9')) {
                            return false;
                        }
                    }
                    return (text.GetLength() > 0);
                };
                if (!isAllDigits(tokens[0]) || !isAllDigits(tokens[2])) {
                    break;
                }
                if (_ttoi(tokens[0]) <= 0 || _ttoi(tokens[2]) <= 0) {
                    break;
                }

                // 在原始 data 中定位 "旧宽 x 旧高" 的起止位置（按空白字符扫描）
                auto isWhitespace = [](TCHAR ch) -> bool {
                    return (ch == _T(' ') || ch == _T('\t') || ch == _T('\r') || ch == _T('\n'));
                };

                int nPos = 0;
                const int nInnerLen = innerText.GetLength();
                while (nPos < nInnerLen && isWhitespace(innerText[nPos])) ++nPos;
                const int nWidthStart = nPos;
                nPos += tokens[0].GetLength();

                while (nPos < nInnerLen && isWhitespace(innerText[nPos])) ++nPos;
                nPos += tokens[1].GetLength();

                while (nPos < nInnerLen && isWhitespace(innerText[nPos])) ++nPos;
                nPos += tokens[2].GetLength();
                if (nPos > nInnerLen) {
                    break;
                }

                // 只替换 [宽度起点, 高度终点) 这一段
                CString newInnerText = innerText.Left(nWidthStart) + newResolution + innerText.Mid(nPos);
                if (FAILED(pTargetData->put_data(CComBSTR(newInnerText)))) {
                    break;
                }
            } else {
                // 3b. 节点为空：新建文本节点写入新分辨率
                CComPtr<IXMLDOMText> pNewTextNode;
                hr = pDoc->createTextNode(CComBSTR(newResolution), &pNewTextNode);
                if (FAILED(hr) || pNewTextNode == NULL) {
                    break;
                }
                CComPtr<IXMLDOMNode> pAppendedNode;
                hr = pVideoModeNode->appendChild(pNewTextNode, &pAppendedNode);
                if (FAILED(hr)) {
                    break;
                }
            }

            // 4. 保存回原文件（MSXML 保留 CDATA、空白与原有编码，不添加 BOM）
            hr = pDoc->save(varPath);
            bResult = SUCCEEDED(hr);
        } while (false);

        if (bNeedUninit) {
            ::CoUninitialize();
        }
        return bResult;
    }

}
