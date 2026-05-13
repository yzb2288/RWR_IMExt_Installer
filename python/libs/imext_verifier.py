# -*- coding: utf-8 -*-
import os
import glob
import json
import fnmatch
import hashlib
import multiprocessing
from pathlib import Path
from copy import deepcopy
from datetime import datetime, timezone, timedelta
from packaging import version as ver
from libs.dll_info import get_dll_custom_tag

FILE_MASK = {
    "IMExt Resources": ["*.dll", "*.cfg", "*.ttf", "*.otf"],
    "Game Exe": ["*.exe"],
    "Game Fonts": [
        "media/packages/vanilla/fonts/*.*",
        "media/packages/vanilla/languages/font_config.xml",
        "media/packages/vanilla/languages/**/font_config.xml"
    ],
    "Localization Files": [
        "media/packages/*/languages/*.*",
        "media/packages/*/languages/**/*.*"
    ]
}

def calculate_md5(file_path):
    hash_md5 = hashlib.md5()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest().lower()

def calculate_md5_task(index, file_rel_path, root_path):
    try:
        md5 = calculate_md5(os.path.join(root_path, file_rel_path))
    except FileNotFoundError:
        md5 = None
    return index, {file_rel_path: md5}

class ImextVerifier:
    def __init__(self, installer_folder_path=None, verify_progress_callback=None):
        self.total_file_num = 0
        self.md5_calculated_num = 0
        self.file_md5_list = []
        
        if installer_folder_path:
            self.install_conf_json_path = os.path.join(installer_folder_path, "install_conf.json")
            if os.path.exists(self.install_conf_json_path) and os.path.isfile(self.install_conf_json_path):
                with open(self.install_conf_json_path, "r", encoding="utf-8") as f:
                    self.install_conf = json.load(f)
                self.install_conf["imext_file_md5_sorted"] = self.sort_file_info(self.install_conf["imext_file_md5"])
            else:
                self.install_conf = {}
        else:
            self.install_conf_json_path = "./install_conf.json"
            self.install_conf = {}
        
        self.verify_progress_callback = verify_progress_callback
        if not callable(self.verify_progress_callback) and self.verify_progress_callback != None:
            raise Exception("verify_progress_callback need to be a callback funciton!")
    
    def sort_file_info(self, file_info_dict):
        file_info_copy = deepcopy(file_info_dict)
        
        file_info_sorted = {}
        for category in list(self.install_conf["file_mask"].keys()):
            category_mask_list = self.install_conf["file_mask"][category]
            file_info_sorted[category] = {}
            for file_rel_path in list(file_info_copy.keys()):
                if any(fnmatch.fnmatch(file_rel_path, mask) for mask in category_mask_list):
                    file_info_sorted[category][file_rel_path] = file_info_copy[file_rel_path]
                    file_info_copy.pop(file_rel_path)
            if len(list(file_info_sorted[category].keys())) == 0:
                file_info_sorted.pop(category)
        
        if len(file_info_copy.keys()) > 0:
            file_info_sorted["Others"] = {}
            file_info_sorted["Others"].update(file_info_copy)
        
        return file_info_sorted
    
    def check_game_compatibility(self, game_folder_path):
        game_exe_md5 = calculate_md5(os.path.join(game_folder_path, "rwr_game.exe"))
        if game_exe_md5 == self.install_conf["target_game_md5"]:
            return True
        elif game_exe_md5 == self.install_conf["imext_file_md5"]["rwr_game.exe"]:
            return True
        else:
            return False
    
    def check_install_version(self, game_folder_path):
        imext_resources_files = self.install_conf["imext_file_md5_sorted"]["IMExt Resources"]
        
        for file_rel_path in list(imext_resources_files.keys()):
            game_file_path = os.path.join(game_folder_path, file_rel_path)
            if not os.path.exists(game_file_path) or not os.path.isfile(game_file_path):
                return False, None
        
        imext_dll_path = os.path.join(game_folder_path, "IMExt.dll")
        imext_ver = get_dll_custom_tag(imext_dll_path, "FileVersion")
        imext_compatible_game_md5 = get_dll_custom_tag(imext_dll_path, "CompatibleGameMD5")
        game_exe_md5 = calculate_md5(os.path.join(game_folder_path, "rwr_game.exe"))
        
        if imext_ver == None:
            if calculate_md5(imext_dll_path) == "b6214e3bb5af8e2979d97e7a31711268":
                if game_exe_md5 == "7a0615b323f780a8a16bf34ea4c2e9ab":
                    return True, "1.0.0"
                else:
                    return False, "1.0.0"
            else:
                return False, None
        
        if game_exe_md5 != imext_compatible_game_md5.lower():
            return False, imext_ver
        else:
            return True, imext_ver
    
    def check_file_md5(self, game_folder_path):
        self.total_file_num = len(self.install_conf["imext_file_md5"].keys())
        if self.total_file_num == 0:
            raise Exception("No files in install_conf.json!")
        
        self.md5_calculated_num = 0
        self.file_md5_list = [None] * self.total_file_num
        
        compare_file_md5 = self.get_files_md5_dict(list(self.install_conf["imext_file_md5"].keys()), game_folder_path)
        
        same_file_dict = {}
        orig_file_dict = {}
        diff_file_dict = {}
        miss_file_dict = {}
        imext_pack_timestamp = datetime.fromisoformat(self.install_conf["timestamp"]).timestamp()
        
        for file_rel_path in list(compare_file_md5.keys()):
            if compare_file_md5[file_rel_path] == self.install_conf["imext_file_md5"][file_rel_path]:
                same_file_dict[file_rel_path] = None
                if file_rel_path in self.install_conf["game_file_md5"].keys() and compare_file_md5[file_rel_path] == self.install_conf["game_file_md5"][file_rel_path]:
                    orig_file_dict[file_rel_path] = None
            elif file_rel_path in self.install_conf["game_file_md5"].keys() and compare_file_md5[file_rel_path] == self.install_conf["game_file_md5"][file_rel_path]:
                orig_file_dict[file_rel_path] = None
            elif compare_file_md5[file_rel_path] != None:
                file_rel_path_obj = Path(file_rel_path)
                if file_rel_path_obj.parts[0] == "media" and os.path.getmtime(os.path.join(game_folder_path, file_rel_path)) > imext_pack_timestamp:
                    diff_file_dict[file_rel_path] = "Newer modify timestamp in game"
                else:
                    diff_file_dict[file_rel_path] = None
            else:
                miss_file_dict[file_rel_path] = None
        
        return self.sort_file_info(same_file_dict), self.sort_file_info(orig_file_dict), self.sort_file_info(diff_file_dict), self.sort_file_info(miss_file_dict)
    
    def create_install_conf_json(
        self,
        file_mask:dict,
        imext_folder_path:str,
        backup_folder_path:str
    ):
        imext_dll_path = os.path.join(imext_folder_path, "IMExt.dll")
        imext_ver = get_dll_custom_tag(imext_dll_path, "FileVersion")
        imext_compatible_game_md5 = get_dll_custom_tag(imext_dll_path, "CompatibleGameMD5").lower()
        v = ver.parse(imext_ver)
        self.install_conf = {
            "version": v.public,
            "timestamp": datetime.now(timezone(timedelta(hours=8))).isoformat(),
            "target_game_md5": imext_compatible_game_md5,
            "file_mask": file_mask,
        }
        
        file_rel_path_list = glob.glob("**/*.*", root_dir=imext_folder_path, recursive=True)
        if not file_rel_path_list:
            raise Exception("No files in imext_folder_path!")
        
        self.total_file_num = len(file_rel_path_list)
        self.md5_calculated_num = 0
        self.file_md5_list = [None] * self.total_file_num
        
        self.install_conf["imext_file_md5"] = self.get_files_md5_dict(file_rel_path_list, imext_folder_path)
        
        file_rel_path_list = glob.glob("**/*.*", root_dir=backup_folder_path, recursive=True)
        if not file_rel_path_list:
            raise Exception("No files in backup_folder_path!")
        
        self.total_file_num = len(file_rel_path_list)
        self.md5_calculated_num = 0
        self.file_md5_list = [None] * self.total_file_num
        
        self.install_conf["game_file_md5"] = self.get_files_md5_dict(file_rel_path_list, backup_folder_path)
        
        #for backup_file_rel_path in list(self.install_conf["game_file_md5"].keys()):
        #    if backup_file_rel_path not in self.install_conf["imext_file_md5"].keys():
        #        self.install_conf["game_file_md5"].pop(backup_file_rel_path)
        
        with open(self.install_conf_json_path, "w", encoding="utf-8") as f:
            json.dump(self.install_conf, f, ensure_ascii=False, indent=2)
        
        self.install_conf["imext_file_md5_sorted"] = self.sort_file_info(self.install_conf["imext_file_md5"])
    
    def get_files_md5_dict_callback(self, index_result):
        idx, res = index_result
        self.file_md5_list[idx] = res
        self.md5_calculated_num += 1
        if self.verify_progress_callback:
            self.verify_progress_callback(self.md5_calculated_num, self.total_file_num)
        else:
            print(f"MD5计算进度: {self.md5_calculated_num}/{self.total_file_num}")
    
    def get_files_md5_dict(self, file_rel_path_list, root_path):
        pool_size = multiprocessing.cpu_count()
        pool = multiprocessing.Pool(processes=pool_size)
        for i, file_rel_path in enumerate(file_rel_path_list):
            pool.apply_async(calculate_md5_task, args=(i, file_rel_path, root_path), callback=self.get_files_md5_dict_callback)
        pool.close()
        pool.join()
        final_dict = {}
        for res in self.file_md5_list:
            final_dict.update(res)
        return final_dict
