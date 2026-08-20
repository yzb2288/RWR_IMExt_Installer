#pragma once
#include <windows.h>
#include <atlstr.h>
#include <atlsimpcoll.h>

namespace StringUtils {
    inline CSimpleArray<CString> Split(const CString& str, const CString& delimiters = _T(" \t")) {
        CSimpleArray<CString> result;
        const int len = str.GetLength();
        int start = 0;

        // 1. 跳过开头的所有连续分隔符
        while (start < len && delimiters.Find(str[start]) != -1) {
            ++start;
        }

        while (start < len) {
            // 2. 从 start 开始向后扫描，直到遇到分隔符或字符串结尾
            int end = start;
            while (end < len && delimiters.Find(str[end]) == -1) {
                ++end;
            }

            // 3. 截取从 start 到 end 之前的子串（此时 end > start，不会为空）
            result.Add(str.Mid(start, end - start));

            if (end == len) {
                // 已经到字符串末尾，结束
                break;
            }

            // 4. 跳过 end 之后连续的分隔符，找到下一个有效字符的起点
            start = end;
            while (start < len && delimiters.Find(str[start]) != -1) {
                ++start;
            }
        }
        return result;
    }
}
