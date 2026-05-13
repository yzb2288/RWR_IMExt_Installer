import ctypes
from ctypes import wintypes

def get_dll_custom_tag(dll_path, tag_name="MyCustomTag"):
    # 1. 加载系统 DLL
    version_dll = ctypes.WinDLL('version.dll')
    
    # 2. 获取版本信息块的大小
    handle = wintypes.DWORD()
    size = version_dll.GetFileVersionInfoSizeW(dll_path, ctypes.byref(handle))
    if not size:
        return None

    # 3. 分配缓冲区并获取完整版本数据
    buffer = ctypes.create_string_buffer(size)
    if not version_dll.GetFileVersionInfoW(dll_path, 0, size, buffer):
        return None

    # 4. 获取语言和编码信息 (用于拼接查询路径)
    # \VarFileInfo\Translation 固定的偏移和结构
    res_ptr = ctypes.c_void_p()
    res_len = wintypes.UINT()
    if not version_dll.VerQueryValueW(buffer, "\\VarFileInfo\\Translation", 
                                     ctypes.byref(res_ptr), ctypes.byref(res_len)):
        return None

    # 解析语言 ID (LangID) 和 字符集 ID (CharsetID)
    # Translation 返回的是一个 4 字节的值：前 2 字节是 LangID，后 2 字节是 CharsetID
    lang_info = ctypes.cast(res_ptr, ctypes.POINTER(ctypes.c_uint32))[0]
    lang = lang_info & 0xFFFF
    charset = (lang_info >> 16) & 0xFFFF
    
    # 5. 拼接路径查询你的自定义字段
    # 格式示例: \StringFileInfo\040904b0\MyCustomTag
    sub_block = f"\\StringFileInfo\\{lang:04x}{charset:04x}\\{tag_name}"
    
    val_ptr = ctypes.c_void_p()
    val_len = wintypes.UINT()
    if version_dll.VerQueryValueW(buffer, sub_block, ctypes.byref(val_ptr), ctypes.byref(val_len)):
        # 将返回的指针转换为字符串
        return ctypes.wstring_at(val_ptr)
    
    return None

if __name__ == "__main__":
    # --- 使用示例 ---
    dll = "./RunningWithRifles/IMExt.dll"
    tag = get_dll_custom_tag(dll, "CompatibleGameMD5")
    print(f"自定义标签内容为: {tag}")
