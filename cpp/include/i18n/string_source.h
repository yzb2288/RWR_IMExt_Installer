#pragma once

// ID(唯一), JSON_KEY(唯一), 英文翻译, 中文翻译
#define STRING_MACROS_TABLE \
X(IDS_KEY_VERSION, "version", "Version", "版本") \
X(IDS_KEY_TIMESTAMP, "timestamp", "Timestamp", "时间戳") \
X(IDS_KEY_TARGET_GAME_MD5, "target_game_md5", "Target Game MD5", "目标游戏MD5") \
X(IDS_KEY_FILE_MASK, "file_mask", "File Mask", "文件掩码") \
X(IDS_KEY_IMEXT_FILE_MD5, "imext_file_md5", "IMExt File MD5", "IMExt文件MD5") \
X(IDS_KEY_GAME_FILE_MD5, "game_file_md5", "Game File MD5", "游戏文件MD5") \
X(IDS_KEY_IMEXT_FILE_MD5_SORTED, "imext_file_md5_sorted", "IMExt File MD5 Sorted", "已分类IMExt文件MD5") \
X(IDS_KEY_CATEGORY_IMEXT_RESOURCES, "IMExt Resources", "IMExt Resources", "IMExt插件文件") \
X(IDS_KEY_CATEGORY_GAME_EXE, "Game Exe", "Game Exe", "游戏EXE文件") \
X(IDS_KEY_CATEGORY_GAME_FONTS, "Game Fonts", "Game Fonts", "游戏字体文件") \
X(IDS_KEY_CATEGORY_LOCALIZATION_FILES, "Localization Files", "Localization Files", "游戏翻译文件") \
X(IDS_KEY_CATEGORY_OTHERS, "Others", "Others", "其他文件") \
X(IDS_KEY_CHECK_RESULT, "check_result", "Check Result", "检查结果") \
X(IDS_KEY_CHECK_RESULT_MISS_FILE, "Missing files", "Missing files", "缺少的文件") \
X(IDS_KEY_CHECK_RESULT_DIFF_FILE, "Mismatched files", "Mismatched files", "不一致的文件") \
X(IDS_KEY_CHECK_RESULT_ORIG_FILE, "Matched game original files", "Matched game original files", "与游戏原文件一致的文件") \
X(IDS_KEY_CHECK_RESULT_SAME_FILE, "Matched IMExt files", "Matched IMExt files", "与IMExt插件一致的文件") \
X(IDS_NOT_RWR_PATH, "not_json_key_0", "Not RWR installation path", "非RWR游戏路径") \
X(IDS_SELECT_RWR_PATH, "not_json_key_1", "Select RWR Path", "选择RWR路径") \
X(IDS_SELECT_BACKUP_PATH, "not_json_key_2", "Select Backup Path", "选择备份路径") \
X(IDS_INSTALL_IMEXT, "not_json_key_3", "Install IMExt", "安装IMExt") \
X(IDS_TITLE_MAIN, "not_json_key_4", "RWR IMExt Installer", "RWR中文输入法插件安装器") \
X(IDS_INCOMPATIBLE_WINDOWS, "not_json_key_5", "Current Windows version is not supported", "不支持当前的Windows系统版本") \
X(IDS_MD5_CHECK_PROGRESS, "not_json_key_6", "MD5 Checking Progress", "MD5校验进度") \
X(IDS_CHECK_INSTALLATION, "not_json_key_7", "Checking installation...", "检查安装状态...") \
X(IDS_INSTALL_STATUS_NOT, "not_json_key_8", "NOT installed", "未安装") \
X(IDS_INSTALL_STATUS_LATEST, "not_json_key_9", "Latest ver installed", "已安装最新版本") \
X(IDS_INSTALL_STATUS_OLD, "not_json_key_10", "Older ver installed", "已安装旧版本") \
X(IDS_NEWER_MODIFY_TIMESTAMP, "not_json_key_11", "Newer modify timestamp in game", "当前游戏文件新于IMExt插件文件") \
X(IDS_INCOMPATIBLE_GAME, "not_json_key_12", "Incompatible", "不兼容") \
X(IDS_INSTALLING, "not_json_key_13", "Installing...", "正在安装...") \
X(IDS_PREPARE_FILE_LIST, "not_json_key_14", "Prepare for file list data", "准备文件列表数据") \
X(IDS_PREPARE_BACKUP, "not_json_key_15", "Prepare for backup", "准备备份文件") \
X(IDS_OK_BTN, "not_json_key_16", "OK", "确定") \
X(IDS_CANCEL_BTN, "not_json_key_17", "Cancel", "取消") \
X(IDS_TITLE_BACKUP, "not_json_key_18", "Backup", "备份文件") \
X(IDS_BACKUP_DESC, "not_json_key_19", "Do you want to create backup in:", "是否在如下目录创建游戏文件备份:") \
X(IDS_BACKUP_ERROR, "not_json_key_20", "Error on backing up!", "备份游戏文件时发生错误!") \
X(IDS_INSTALL_IMEXT_FILES_DESC, "not_json_key_21", "Install imext files to game folder:", "安装IMExt插件文件到游戏目录:") \
X(IDS_INSTALL_IMEXT_ERROR, "not_json_key_22", "Error on installing IMExt!", "安装IMExt插件时出错!") \
X(IDS_INSTALL_IMEXT_FONTS_DESC, "not_json_key_23", "Install imext fonts to game folder:", "安装字体文件到游戏目录:") \
X(IDS_INSTALL_LOCALIZATION_FILES_DESC, "not_json_key_24", "Install utf-8 localization files to game folder:", "安装经过UTF8转换的游戏翻译文件到游戏目录:") \
X(IDS_INSTALL_LOCALIZATION_FILES_DESC_NOTE, "not_json_key_25", "(For users with Chinese language settings, this step can be bypassed)", "(对于游戏设置了中文语言的用户可以跳过此步骤)") \
X(IDS_TITLE_NOTICE, "not_json_key_26", "Notice", "注意") \
X(IDS_TITLE_ERROR, "not_json_key_27", "Error", "错误") \
X(IDS_TITLE_COPY, "not_json_key_28", "Copying", "复制文件") \
X(IDS_DX9_NOTICE, "not_json_key_29", "You have enabled DX9 rendering mode, which may conflict with the Steam Overlay and cause the game to crash. It is suggested to switch to OpenGL mode, or try disabling Steam Overlay in the RWR's Steam properties.", "检测到游戏目前使用DirectX9渲染模式, 在该模式下插件有一定概率跟Steam游戏内叠加界面功能冲突. 强烈建议切换到OpenGL渲染模式, 或者尝试在Steam的RWR属性中关闭Steam叠加界面功能") \
X(IDS_INSTALL_SUCCESS, "not_json_key_30", "Successfully installed IMExt!", "成功安装IMExt插件!") \
X(IDS_INSTALL_SUCCESS_ALT, "not_json_key_31", "Successfully run as you need!", "成功按照选择步骤运行!") \
X(IDS_EXPAND_ALL, "not_json_key_32", "Expand All", "全部展开") \
X(IDS_COLLAPSE_ALL, "not_json_key_33", "Collapse All", "全部缩回") \
X(IDS_OPEN_IN_RWR, "not_json_key_34", "Open Game file", "打开游戏目录文件") \
X(IDS_OPEN_IN_IMEXT, "not_json_key_35", "Open IMExt file", "打开插件目录文件") \
X(IDS_SHOW_IN_RWR, "not_json_key_36", "Show Game file in explorer", "在文件夹中显示游戏目录文件") \
X(IDS_SHOW_IN_IMEXT, "not_json_key_37", "Show IMExt file in explorer", "在文件夹中显示插件目录文件") \
X(IDS_INSTALL_STATUS_NEW, "not_json_key_38", "Newer ver installed", "已安装更新版本") \

// 不可直接换行, 必须用反斜杠\换行
// 不可将EOF放到STRING_MACROS_TABLE末尾一行
