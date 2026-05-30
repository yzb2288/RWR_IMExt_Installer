#include "core/font_resize_tool.h"

// 强制 stb_image 只编译 PNG 解码器
#define STBI_ONLY_PNG
#define __STDC_LIB_EXT1__
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "libs/stb/stb_image_resize2.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb/stb_image_write.h"

bool FontResizeTool::ScanFonts() {
    m_fontPngFileNameList.RemoveAll();
    m_fontPngOldResolutionList.RemoveAll();
    m_fontPngNewResolutionList.RemoveAll();
    m_fontPngResizeProgressStatus.RemoveAll();

    if (!FileUtils::FileExists(m_rwrFontsFolderPath) || FileUtils::IsFile(m_rwrFontsFolderPath)) {
        return false;
    }

    FileUtils::GetFileRelPathListByMask(m_rwrFontsFolderPath, m_fontPngFileMask, m_fontPngFileNameList);

    m_maxPixelNum = 0ll;
    int count = m_fontPngFileNameList.GetSize();
    for (int i = 0; i < count; i++) {
        CSize pngSize;
        if (!GetPngSize(m_rwrFontsFolderPath + _T("\\") + m_fontPngFileNameList[i], pngSize)) {
            m_fontPngFileNameList.RemoveAll();
            m_fontPngOldResolutionList.RemoveAll();
            m_fontPngNewResolutionList.RemoveAll();
            m_fontPngResizeProgressStatus.RemoveAll();
            return false;
        }

        m_fontPngOldResolutionList.Add(pngSize);
        m_fontPngNewResolutionList.Add(pngSize);
        m_fontPngResizeProgressStatus.Add(ResizeProgressStatus::Idle);

        LONGLONG pngPixelNum = (LONGLONG)pngSize.cx * (LONGLONG)pngSize.cy;
        if (pngPixelNum > m_maxPixelNum) {
            m_maxPixelNum = pngPixelNum;
        }
    }

    return true;
}

// 当前为按边长缩放的百分比, 便于用户直观理解
bool FontResizeTool::SetTargetResolutionPercentage(int per) {
    if ((m_fontPngFileNameList.GetSize() != m_fontPngOldResolutionList.GetSize()) ||
        (m_fontPngFileNameList.GetSize() != m_fontPngNewResolutionList.GetSize()) ||
        (m_fontPngFileNameList.GetSize() != m_fontPngResizeProgressStatus.GetSize())
    ) {
        return false;
    }

    // 用像素面积作为判断阈值
    LONGLONG resizeThreshold = m_maxPixelNum * (LONGLONG)per * (LONGLONG)per / 10000ll;
    int count = m_fontPngOldResolutionList.GetSize();
    for (int i = 0; i < count; i++) {
        // 重置resize状态为Idle, 需要适配执行resize之后的状态更新, 如果当前已经resize成功, 则修改旧分辨率为resize之后的
        if (m_fontPngResizeProgressStatus[i] == ResizeProgressStatus::Succeeded) {
            CSize pngSize;
            if (!GetPngSize(m_rwrFontsFolderPath + _T("\\") + m_fontPngFileNameList[i], pngSize)) {
                m_fontPngFileNameList.RemoveAll();
                m_fontPngOldResolutionList.RemoveAll();
                m_fontPngNewResolutionList.RemoveAll();
                m_fontPngResizeProgressStatus.RemoveAll();
                return false;
            }
            m_fontPngOldResolutionList[i] = pngSize;
            m_fontPngNewResolutionList[i] = pngSize;
        }
        m_fontPngResizeProgressStatus[i] = ResizeProgressStatus::Idle;

        LONGLONG pngPixelNum = (LONGLONG)m_fontPngOldResolutionList[i].cx * (LONGLONG)m_fontPngOldResolutionList[i].cy;
        if (pngPixelNum > resizeThreshold) {
            // 像素面积大于阈值像素面积, 则按原比例resize到最接近阈值的尺寸
            m_fontPngNewResolutionList[i].cy = (LONG)sqrt(
                (double)resizeThreshold * (double)m_fontPngOldResolutionList[i].cy / (double)m_fontPngOldResolutionList[i].cx
            );
            m_fontPngNewResolutionList[i].cx = m_fontPngNewResolutionList[i].cy * m_fontPngOldResolutionList[i].cx / m_fontPngOldResolutionList[i].cy;
        } else {
            m_fontPngNewResolutionList[i].cx = m_fontPngOldResolutionList[i].cx;
            m_fontPngNewResolutionList[i].cy = m_fontPngOldResolutionList[i].cy;
        }
    }

    return true;
}

bool FontResizeTool::ResizeFonts(ResizeProgressCallback callback, void* pCaller) {
    if ((m_fontPngFileNameList.GetSize() != m_fontPngOldResolutionList.GetSize()) ||
        (m_fontPngFileNameList.GetSize() != m_fontPngNewResolutionList.GetSize()) ||
        (m_fontPngFileNameList.GetSize() != m_fontPngResizeProgressStatus.GetSize())
    ) {
        return false;
    }
    
    int count = m_fontPngFileNameList.GetSize();
    CSimpleArray<int> resizeIndexList;
    for (int i = 0; i < count; i++) {
        if (m_fontPngNewResolutionList[i] != m_fontPngOldResolutionList[i]) {
            resizeIndexList.Add(i);
        }
    }

    int result = true;
    int resizeCount = resizeIndexList.GetSize();
    for (int j = 0; j < resizeCount; j++) {
        m_fontPngResizeProgressStatus[resizeIndexList[j]] = ResizeProgressStatus::Resizing;
        if (callback) {
            callback(j + 1, resizeCount, ResizeProgressStatus::Resizing, pCaller);
        }

        if (ResizePng(m_rwrFontsFolderPath + _T("\\") + m_fontPngFileNameList[resizeIndexList[j]], m_fontPngNewResolutionList[resizeIndexList[j]], true)) {
            m_fontPngResizeProgressStatus[resizeIndexList[j]] = ResizeProgressStatus::Succeeded;
            if (callback) {
                callback(j + 1, resizeCount, ResizeProgressStatus::Succeeded, pCaller);
            }
        } else {
            m_fontPngResizeProgressStatus[resizeIndexList[j]] = ResizeProgressStatus::Failed;
            if (callback) {
                callback(j + 1, resizeCount, ResizeProgressStatus::Failed, pCaller);
            }
            result = false;
        }
    }
    
    return result;
}

bool FontResizeTool::GetPngSize(const CString& pngPath, CSize& size) {
    int width = 0;
    int height = 0;
    int channels = 0;

    int success = stbi_info(CT2A(pngPath), &width, &height, &channels);

    if (success) {
        size.cx = width;
        size.cy = height;
        return true;
    } else {
        return false;
    }
}

bool FontResizeTool::ResizePng(const CString& pngPath, const CSize& newSize, bool overwrite) {
    // 默认压缩等级0 - 9, 0不压缩, 8是stb默认值
    stbi_write_png_compression_level = 8;

    CString tempSuffix = _T(".resized.png");
    CString tempOutPutPath = pngPath + tempSuffix;
    int tempIndex = 1;
    while (FileUtils::FileExists(tempOutPutPath)) {
        tempSuffix.Format(_T(".resized_%d.png"), tempIndex);
        tempOutPutPath = pngPath + tempSuffix;
        tempIndex++;
    }
    
    // 目标尺寸
    int targetWidth = newSize.cx;
    int targetHeight = newSize.cy;

    // 2. 加载 PNG 图片到内存 (获取宽、高、通道数)
    int width, height, channels;
    unsigned char* inputPixels = stbi_load(CT2A(pngPath), &width, &height, &channels, 0);
    if (!inputPixels) {
        //std::cerr << "无法加载输入图片: " << input_path << std::endl;
        return false;
    }

    // 3. 分配目标图像的内存
    unsigned char* outputPixels = (unsigned char*)malloc(targetWidth * targetHeight * channels);
    if (!outputPixels) {
        //std::cerr << "内存分配失败" << std::endl;
        stbi_image_free(inputPixels);
        return false;
    }

    // stbir_resize_uint8_linear 会自动适配通道数，且对 Alpha 透明通道有极好的支持
    unsigned char* result = (unsigned char *)stbir_quick_resize_helper(
        inputPixels, width, height, 0,                // 输入数据、宽高、输入步长(0为自动)
        outputPixels, targetWidth, targetHeight, 0,   // 输出数据、目标宽高、输出步长
        (stbir_pixel_layout)channels,                 // 传入通道数 (如 3=RGB, 4=RGBA)
        STBIR_TYPE_UINT8_SRGB,                        // 数据类型与色彩空间 (8位无符号，RGB用sRGB，Alpha线性)
        STBIR_EDGE_CLAMP,                             // 边缘处理
        STBIR_FILTER_CATMULLROM                       // 核心：Catmull-Rom 滤镜 (带来高锐度)
    );

    if (!result) {
        //std::cerr << "图片缩放失败" << std::endl;
        free(outputPixels);
        stbi_image_free(inputPixels);
        return false;
    }

    // 5. 将缩放后的像素数组保存为新的 PNG 文件
    int stride_in_bytes = targetWidth * channels;
    int write_success = stbi_write_png(CT2A(tempOutPutPath), targetWidth, targetHeight, channels, outputPixels, stride_in_bytes);
    
    if (!write_success) {
        //std::cerr << "保存 PNG 失败" << std::endl;
        free(outputPixels);
        stbi_image_free(inputPixels);
        return false;
    }

    // 6. 释放内存
    free(outputPixels);
    stbi_image_free(inputPixels);

    // 覆盖原文件
    if (overwrite) {
        if (FileUtils::CopyFileWithMakedirs(tempOutPutPath, pngPath, true)) {
            DeleteFile(tempOutPutPath);
        } else {
            return false;
        }
    }

    return true;
}