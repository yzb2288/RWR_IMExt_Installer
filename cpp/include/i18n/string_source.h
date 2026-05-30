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
X(IDS_MISMATCHED_VERSION, "not_json_key_12", "Version Mismatch", "版本不一致") \
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
X(IDS_GAME_EXE_STATUS_ERROR, "not_json_key_39", "Game EXE status error", "游戏EXE状态异常") \
X(IDS_OLD_WIN_DESC, "not_json_key_40", "Current system is below Windows 10. Some plugin modes may not work. See imext_config.ini after installation for details. Continue?", "当前系统版本低于Windows10, 插件部分模式可能无法生效, 具体说明请查看安装后的imext_config.ini文件, 是否继续?") \
X(IDS_CANCEL_INSTALL, "not_json_key_41", "Cancel installation!", "取消安装!") \
X(IDS_NEWER_VERSION_OVERWRITE_INSTALL_DESC, "not_json_key_42", "Installed version is newer than the installation version. Continue to OVERWRITE install?", "当前已安装版本的版本号高于待安装版本, 是否继续覆盖安装?") \
X(IDS_TITLE_WARNING, "not_json_key_43", "Warning", "警告") \
X(IDS_UNKNOWN_EXE_WARNING_DESC, "not_json_key_44", "Game EXE has been modified (by an official update or another program). Continuing may cause the game to crash or fail to start. Continue to OVERWRITE install?", "游戏EXE已被修改, 可能是游戏更新或者被其他程序修改, 继续安装可能导致游戏无法启动或崩溃, 是否继续覆盖安装?") \
X(IDS_KEY_GAME_VERSION, "game_version", "Target game version for this installation", "本次安装对应的游戏版本") \
X(IDS_KEY_LAST_GAME_UPDATE_TIMESTAMP, "last_game_update_timestamp", "Target game update date for this installation", "本次安装对应的游戏更新日期") \
X(IDS_PATCH_TIME_NEWER_DESC, "not_json_key_45", "Installed version's build date is newer than the installation version. A newer version might already exist. Continue to OVERWRITE install?", "当前已安装版本的编译日期晚于待安装版本, 可能已经安装了新版本, 是否继续覆盖安装?") \
X(IDS_IMEXT_PATCH_TIME, "not_json_key_46", "Buid date for this installation", "本次安装插件的编译日期") \
X(IDS_INSTALLED_PATCH_TIME, "not_json_key_47", "Buid date for installed version", "已安装插件的编译日期") \
X(IDS_IMEXT_INI_CONFIG_PATH_NOTICE, "not_json_key_48", "Plugin config path is shown below. Highly recommended to check before use. Open it now:", "插件配置文件路径如下, 强烈建议使用前查阅, 是否打开:") \
X(IDS_TOOLTIP_ON_BTN_INSTALL_IMEXT, "not_json_key_49", "Right-click to open toolbar", "右键打开工具栏") \
X(IDS_RWR_FONT_RESIZE_TOOL, "not_json_key_50", "RWR Font PNG resize tool", "RWR字体PNG尺寸调节工具") \
X(IDS_SEARCH_PNG_FAILED, "not_json_key_51", "Font PNG files NOT found!", "当前目录无法找到字体PNG文件!") \
X(IDS_RWR_FONT_RESIZE_TOOL_DESC, "not_json_key_52", "If you experience any of the following common issues in the game, please use the slider below to adjust the font size (Recommendation: Start at 6000x6000 and gradually decrease it). Please note that this action cannot be undone!", "当你在游戏中遇到如下典型问题时, 请进行通过下方滑条进行字体尺寸调整(建议先从6000x6000的分辨率开始逐步调小), 注意此操作不可逆!") \
X(IDS_RESIZE_PNG_PROGRESS_STATUS_RESIZING, "not_json_key_53", "Resizing", "计算中") \
X(IDS_RESIZE_PNG_PROGRESS_STATUS_SUCCEEDED, "not_json_key_54", "Succeeded", "成功") \
X(IDS_RESIZE_PNG_PROGRESS_STATUS_FAILED, "not_json_key_55", "Failed", "失败") \
X(IDS_RWR_FONT_PROBLEM_DESC, "not_json_key_56", "1. Game crashes occur when the loading screen reaches 80%–84%.\n2. Chatbubble or Chatlog text is invisible.\n3. Crashes or \"bad allocation\" pop-ups appear while starting switching maps.\n4. Long map loading times result in disconnections.", "1. 过图加载到80-84%阶段报错\n2. 对话框字体看不见\n3. 加载地图时报错或者弹出bad allocation弹窗\n4. 过图加载时间过长导致掉线") \
X(IDS_RWR_FONT_PROBLEM_NOTICE_A, "not_json_key_57", "When using integrated graphics or a dual-GPU laptop (with or without direct GPU ouput), there is a certain probability of encountering the following issues:", "当你使用CPU核显、双显卡笔记本(无论是否独显直通)时, 会有一定概率遇到下面的问题:") \
X(IDS_RWR_FONT_PROBLEM_NOTICE_B, "not_json_key_58", "These are usually because the current IMExt font size is too large, causing the font texture to fail to load. To fix this, right-click the \"Install IMExt\" button and open the \"RWR Font PNG resize tool\".", "这通常是由于当前IMExt的字体尺寸太大导致字体纹理加载失败导致的, 需要在\"安装IMExt\"按钮处右键打开\"RWR字体PNG尺寸调节工具\"进行解决") \

// 不可直接换行, 必须用反斜杠\换行
// 不可将EOF放到STRING_MACROS_TABLE末尾一行
