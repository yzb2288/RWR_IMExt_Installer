#pragma once
#include <atlbase.h>
#include <atlstr.h>

#include "i18n/string_source.h"

// 翻译宏
#define _TR(nID) I18n::tr(nID)
// 获取utf8 json key
#define _KU8(nID) I18n::stringKeyU8[nID]
// 通过utf8 key获取翻译
#define _K2TR(key) I18n::GetUtf8KeyI18nStr(key)

enum StringId {
    #define X(id, key, en, zh) id,
        STRING_MACROS_TABLE
    #undef X
    IDS_COUNT
};

namespace I18n {
    //static LANGID lid = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    static LANGID lid = GetUserDefaultUILanguage();

    static LPCTSTR stringEN[StringId::IDS_COUNT] = {
        #define X(id, key, en, zh) _T(en),
            STRING_MACROS_TABLE
        #undef X
    };

    static LPCTSTR stringZH[StringId::IDS_COUNT] = {
        #define X(id, key, en, zh) _T(zh),
            STRING_MACROS_TABLE
        #undef X
    };

    static const char* stringKeyU8[StringId::IDS_COUNT] = {
        #define X(id, key, en, zh) u8##key,
            STRING_MACROS_TABLE
        #undef X
    };

    constexpr unsigned int OffsetBasis = 2166136261u;
    constexpr unsigned int Prime = 16777619u;

    constexpr unsigned int CalculateKeyHash(const char* str, unsigned int last_hash = OffsetBasis) {
        return *str ? CalculateKeyHash(str + 1, (last_hash ^ static_cast<unsigned char>(*str)) * Prime) : last_hash;
    }

    constexpr unsigned int operator "" _hash(const char* str, size_t) {
        return CalculateKeyHash(str);
    }

    inline LPCTSTR tr(StringId nID) {
        switch (PRIMARYLANGID(lid))
        {
        case LANG_CHINESE:
            return stringZH[nID];
        default:
            return stringEN[nID];
        }
        return _T("");
    }

    inline CString GetUtf8KeyI18nStr(const char* key) {
        UINT h = CalculateKeyHash(key);

        // 使用 switch-case，編譯器會優化成跳轉表（Jump Table），效率極高
        switch (h) {
            #define X(id, key, eng, chn) \
                case CalculateKeyHash(key): \
                    return tr(StringId::id);
            
            STRING_MACROS_TABLE
            #undef X
            default:
                // 如果沒匹配到，將 UTF-8 轉為 Unicode 原樣顯示
                CString fallback(CA2T(key, CP_UTF8));
                return fallback;
        }
    }

    inline void RefreshLanguage() {
        lid = GetUserDefaultUILanguage();
    }
};