import gettext
import locale
from pathlib import Path

locales_dir = Path(__file__).parent / "locales"

# 支持的语言列表
supported_language = []
for locale_path in Path(locales_dir).glob("*"):
    if locale_path.is_dir():
        locale_name = locale_path.name
        supported_language.append(locale_name)

# 默认语言
DEFAULT_LANG = "en_US"

# 全局翻译函数（初始化前先给个透传，避免导入顺序问题）
_ = lambda s: s

def get_system_language():
    """尝试获取系统语言，匹配支持列表"""
    try:
        sys_lang = locale.getdefaultlocale()[0]  # 例如 "zh_CN"
        if sys_lang in supported_language:
            return sys_lang
        # 只匹配语言前缀，比如 "zh" 匹配 "zh_CN"
        prefix = sys_lang.split("_")[0] if sys_lang else ""
        for lang in supported_language:
            if lang.startswith(prefix):
                return lang
    except Exception:
        pass
    return DEFAULT_LANG

def setup_i18n(lang=None):
    """
    初始化国际化配置。
    lang: 指定语言代码，None则自动检测系统语言
    返回翻译函数 _
    """
    global _
    
    if lang is None:
        lang = get_system_language()
    
    if lang not in supported_language:
        lang = DEFAULT_LANG
    
    try:
        translation = gettext.translation(
            domain="messages",
            localedir=str(locales_dir),
            languages=[lang]
        )
        translation.install()
        _ = translation.gettext
    except FileNotFoundError:
        _ = lambda s: s
    
    return _
