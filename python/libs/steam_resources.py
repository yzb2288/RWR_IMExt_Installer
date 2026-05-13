# -*- coding: utf-8 -*-
import os
import vdf
import ctypes
from ctypes import wintypes

class SteamResources:
    def __init__(self):
        self.steam_path = self.get_steam_path()
        with open(os.path.join(self.steam_path, "steamapps\\libraryfolders.vdf"), "r", encoding="utf-8") as f:
            self.steam_library_folders = vdf.loads(f.read())
    
    def get_steam_path(self):
        # 定义 Windows 常量
        HKEY_CURRENT_USER = 0x80000001
        KEY_READ = 0x20019
        ERROR_SUCCESS = 0

        # 加载 advapi32.dll
        advapi32 = ctypes.windll.advapi32
        
        h_key = wintypes.HANDLE()
        sub_key = r"Software\Valve\Steam"
        value_name = "SteamPath"

        # 1. 打开注册表项
        # 使用 W 结尾的函数 (RegOpenKeyExW) 以支持 Unicode
        if advapi32.RegOpenKeyExW(HKEY_CURRENT_USER, sub_key, 0, KEY_READ, ctypes.byref(h_key)) != ERROR_SUCCESS:
            return None

        try:
            # 2. 获取缓冲区所需的大小
            size = wintypes.DWORD()
            advapi32.RegQueryValueExW(h_key, value_name, None, None, None, ctypes.byref(size))

            # 3. 创建缓冲区并读取数据
            # size.value 是字节数，Unicode 字符需除以 2（或者直接创建相应字节的缓冲区）
            buf = ctypes.create_unicode_buffer(size.value // 2)
            if advapi32.RegQueryValueExW(h_key, value_name, None, None, ctypes.cast(buf, ctypes.POINTER(wintypes.BYTE)), ctypes.byref(size)) == ERROR_SUCCESS:
                return buf.value
        finally:
            # 4. 关闭句柄
            advapi32.RegCloseKey(h_key)
        
        return None
    
    def get_appmanifest_path_from_app_id(self, game_app_id):
        for library_folders_id in self.steam_library_folders["libraryfolders"].keys():
            for app_id in self.steam_library_folders["libraryfolders"][library_folders_id]["apps"].keys():
                if app_id == str(game_app_id):
                    game_library_folder_path = self.steam_library_folders["libraryfolders"][library_folders_id]["path"]
                    game_steamapps_folder_path = os.path.join(game_library_folder_path, "steamapps")
                    for file in os.listdir(game_steamapps_folder_path):
                        file_path = os.path.join(game_steamapps_folder_path, file)
                        if os.path.isfile(file_path) and file == f"appmanifest_{game_app_id}.acf":
                            return file_path
                    break
        raise Exception(f"Steam game id {game_app_id} is not installed!")
    
    def get_game_path_from_app_id(self, game_app_id):
        game_appmanifest_path = self.get_appmanifest_path_from_app_id(game_app_id)
        steamapps_folder_path, tail = os.path.split(game_appmanifest_path)
        with open(game_appmanifest_path, "r", encoding="utf-8") as f:
            game_appmanifest = vdf.loads(f.read())
        return os.path.join(steamapps_folder_path, "common\\" + game_appmanifest["AppState"]["installdir"])
    
    def get_localconfig_path_from_app_last_owner(self, game_app_id):
        game_appmanifest_path = self.get_appmanifest_path_from_app_id(game_app_id)
        with open(game_appmanifest_path, "r", encoding="utf-8") as f:
            game_appmanifest = vdf.loads(f.read())
        steamid64 = game_appmanifest["AppState"]["LastOwner"]
        steamid32 = self.steam64_to_32(steamid64)
        last_owner_localconfig_path = os.path.join(self.steam_path, f"userdata/{steamid32}/config/localconfig.vdf")
        return last_owner_localconfig_path
    
    def get_steam_overlay_enable_status_from_app_last_owner(self, game_app_id):
        last_owner_localconfig_path = self.get_localconfig_path_from_app_last_owner(game_app_id)
        with open(last_owner_localconfig_path, "r", encoding="utf-8") as f:
            localconfig = vdf.loads(f.read())
        try:
            status = localconfig["UserLocalConfigStore"]["apps"][str(game_app_id)]["OverlayAppEnable"]
            if status == "1":
                return True
            else:
                return False
        except KeyError:
            return True
        

    def steam64_to_32(self, steam64):
        steam64 = int(steam64)
        BASE_ID = 76561197960265728
        return str(steam64 - BASE_ID)

if __name__ == "__main__":
    sr = SteamResources()
    rwr_install_folder_path = sr.get_game_path_from_app_id(270150)
    a = sr.get_steam_overlay_enable_status_from_app_last_owner(270150)
    rwr_appdata_folder_path = os.path.join(os.getenv("APPDATA"), "Running with rifles")
    print(rwr_install_folder_path)
    print(rwr_appdata_folder_path)