import os
import sys

script_folder_path = os.path.split(os.path.realpath(sys.argv[0]))[0]
sys.path.append(script_folder_path)

import shutil
import platform
import subprocess
import threading
from pathlib import Path
from datetime import datetime
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from packaging.version import Version
from libs.i18n import setup_i18n
from libs.steam_resources import SteamResources
from libs.rwr_config import RWRConfigManager
from libs.imext_verifier import ImextVerifier

global_translation = setup_i18n()

# 内核版本大于6.1可以判断大于Win7 (Win8是6.2)
INCOMPATIBLE_WIN_VER = "6.1"
def win_ver_compatibility():
    if platform.system() == "Windows":
        win_ver = Version(platform.win32_ver()[1])
        compare_ver = Version(INCOMPATIBLE_WIN_VER)
        if win_ver > compare_ver:
            return True
        else:
            return False

class RwrImextInstallerApp:
    def __init__(self, root:tk.Tk):
        self._ = global_translation
        self.root = root
        
        self.root.columnconfigure(0, weight=1)
        self.root.columnconfigure(1, weight=4)
        #self.root.rowconfigure(0, weight=1)
        #self.root.rowconfigure(1, weight=1)
        #self.root.rowconfigure(2, weight=1)
        self.root.rowconfigure(3, weight=1)
        
        self.is_checking_imext_install_status = False
        self.check_imext_install_status_thread = None
        self.game_imext_install_status = False
        self.game_folder_imext_verion = None
        self.same_file_dict = {}
        self.orig_file_dict = {}
        self.diff_file_dict = {}
        self.miss_file_dict = {}
        self.is_installing_imext = False
        self.install_imext_thread = None
        
        self.imext_path = os.path.join(script_folder_path, "RunningWithRifles")
        
        rwr_path = os.path.join(script_folder_path, "../../../../common/RunningWithRifles")
        if os.path.exists(rwr_path) and os.path.isdir(rwr_path):
            self.rwr_install_path = tk.StringVar(self.root, value=os.path.realpath(rwr_path))
        else:
            try:
                self.steam_resources = SteamResources()
                self.rwr_install_path = tk.StringVar(self.root, value=self.steam_resources.get_game_path_from_app_id(270150))
            except Exception as e:
                self.rwr_install_path = tk.StringVar(self.root)
        
        try:
            self.rwr_appdata_folder_path = os.path.join(os.getenv("APPDATA"), "Running with rifles")
        except Exception as e:
            self.rwr_appdata_folder_path = None
            self.rwr_config_manager = None
        else:
            if os.path.exists(os.path.join(self.rwr_appdata_folder_path, "config.xml")):
                self.rwr_config_manager = RWRConfigManager(self.rwr_appdata_folder_path)
            else:
                self.rwr_config_manager = None
        
        self.update_rwr_install_path_button = ttk.Button(self.root, text=self._("Select RWR Path"), command=self.update_rwr_install_path)
        self.update_rwr_install_path_button.grid(column=0, row=0, sticky="nsew", padx=(10, 5))
        
        self.rwr_install_path_entry = ttk.Entry(self.root, textvariable=self.rwr_install_path, width=40, state="readonly")
        self.rwr_install_path_entry.grid(column=1, row=0, sticky="nsew", padx=(5, 10))
        
        self.update_backup_path_button = ttk.Button(self.root, text=self._("Select Backup Path"), command=self.update_backup_path, state=tk.DISABLED)
        self.update_backup_path_button.grid(column=0, row=1, sticky="nsew", padx=(10, 5))
        
        self.backup_path = tk.StringVar(self.root, value=os.path.realpath("./backup"))
        self.backup_path_entry = ttk.Entry(self.root, textvariable=self.backup_path, width=40, state="readonly")
        self.backup_path_entry.grid(column=1, row=1, sticky="nsew", padx=(5, 10))
        
        self.imext_install_button = ttk.Button(self.root, text=self._("Install IMExt"), command=self.start_install_imext, state=tk.DISABLED)
        self.imext_install_button.grid(column=0, row=2, sticky="nsew", padx=(10, 5))
        
        self.imext_install_status_text = tk.StringVar(self.root, value=self._("Not RWR installation path"))
        self.imext_install_status_label = ttk.Label(self.root, textvariable=self.imext_install_status_text, foreground="red")
        self.imext_install_status_label.grid(column=1, row=2, sticky="w", padx=(5, 10))
        
        self.file_tree_container = ttk.Frame(self.root)
        self.file_tree_container.columnconfigure(0, weight=1)
        self.file_tree_container.rowconfigure(0, weight=1)
        self.file_tree = ttk.Treeview(self.file_tree_container, columns=["game_file_path", "imext_file_path"], displaycolumns=(), show=["tree"])
        self.file_tree.tag_configure("MissingFiles", foreground="red")
        self.file_tree.tag_configure("DiffFiles", foreground="chocolate")
        self.file_tree.tag_configure("OrigFiles", foreground="darkgreen")
        self.file_tree.tag_configure("SameFiles", foreground="green")
        self.file_tree.bind("<Button-3>", lambda event: self.show_file_context_menu(event, self.file_tree))
        self.file_tree_scrollbar = ttk.Scrollbar(self.file_tree_container, orient="vertical", command=self.file_tree.yview)
        self.file_tree.configure(yscrollcommand=self.file_tree_scrollbar.set)
        self.file_tree.grid(column=0, row=0, sticky="nsew")
        self.file_tree_scrollbar.grid(column=1, row=0, sticky="ns")
        
        self.progress_text = tk.StringVar(self.root)
        self.progress_label = ttk.Label(self.root, textvariable=self.progress_text)
        
        self.imext_verifier = ImextVerifier(script_folder_path, self.update_md5_check_progress)
        self.root.title("{} - IMExt {}".format(
            self._("RWR IMExt Installer"),
            self.imext_verifier.install_conf["version"]
        ))
        
        if win_ver_compatibility():
            if os.path.exists(os.path.join(self.rwr_install_path.get(), "rwr_game.exe")):
                self.update_imext_install_status()
        else:
            self.imext_install_status_text.set(self._("Current Windows version is not supported"))
            self.imext_install_status_label.configure(foreground="red")
            self.update_rwr_install_path_button.configure(state=tk.DISABLED)
            self.update_backup_path_button.configure(state=tk.DISABLED)
            self.imext_install_button.configure(state=tk.DISABLED)
    
    def update_rwr_install_path(self):
        result = filedialog.askdirectory(title=self._("Select RWR Installation Path"))
        if result:
            rwr_path = os.path.normpath(result)
            if rwr_path and not os.path.samefile(rwr_path, self.rwr_install_path.get()):
                self.file_tree.delete(*self.file_tree.get_children())
                self.file_tree_container.grid_forget()
                self.root.geometry("")
                self.update_backup_path_button.configure(state=tk.DISABLED)
                self.imext_install_button.configure(state=tk.DISABLED)
                
                self.rwr_install_path.set(rwr_path)
                if os.path.exists(os.path.join(rwr_path, "rwr_game.exe")):
                    self.update_imext_install_status()
                else:
                    self.imext_install_status_text.set(self._("Not RWR installation path"))
                    self.imext_install_status_label.configure(foreground="red")
    
    def update_backup_path(self):
        result = filedialog.askdirectory(title=self._("Select Backup Path"))
        if result:
            new_backup_path = os.path.normpath(result)
            if new_backup_path:
                self.backup_path.set(new_backup_path)
    
    def update_imext_install_status(self):
        if not self.is_checking_imext_install_status:
            self.is_checking_imext_install_status = True
            self.game_imext_install_status = False
            self.game_folder_imext_verion = None
            self.update_rwr_install_path_button.configure(state=tk.DISABLED)
            self.imext_install_status_text.set(self._("Checking installation..."))
            self.imext_install_status_label.configure(foreground="black")
            self.update_progress_text(self._("Prepare for checking"))
            if self.check_imext_install_status_thread:
                self.check_imext_install_status_thread.join()
            self.check_imext_install_status_thread = threading.Thread(target=self.check_imext_install_status)
            self.check_imext_install_status_thread.daemon = True
            self.check_imext_install_status_thread.start()
            
    
    def check_imext_install_status(self):
        rwr_path = self.rwr_install_path.get()
        last_text_color = "black"
        self.game_imext_install_status, self.game_folder_imext_verion = self.imext_verifier.check_install_version(rwr_path)
        if not self.game_imext_install_status:
            self.imext_install_status_text.set(self._("NOT installed"))
            last_text_color = "red"
        else:
            self.imext_install_status_text.set("{} {}".format(
                self._("Installed"),
                self.game_folder_imext_verion
            ))
            if self.game_folder_imext_verion == self.imext_verifier.install_conf["version"]:
                last_text_color = "green"
            else:
                last_text_color = "orange"

        self.same_file_dict, self.orig_file_dict, self.diff_file_dict, self.miss_file_dict = self.imext_verifier.check_file_md5(rwr_path)
        if "IMExt Resources" in self.diff_file_dict.keys():
            for file_rel_path in self.diff_file_dict["IMExt Resources"].keys():
                if self.game_folder_imext_verion:
                    self.diff_file_dict["IMExt Resources"][file_rel_path] = self.game_folder_imext_verion
                else:
                    self.diff_file_dict["IMExt Resources"][file_rel_path] = self._("Unknown version")
        self.update_md5_check_result_tree(self.same_file_dict, self.orig_file_dict, self.diff_file_dict, self.miss_file_dict)
        
        comp = self.imext_verifier.check_game_compatibility(rwr_path)
        if comp:
            self.update_backup_path_button.configure(state=tk.NORMAL)
            self.imext_install_button.configure(state=tk.NORMAL)
        else:
            self.update_backup_path_button.configure(state=tk.DISABLED)
            self.imext_install_button.configure(state=tk.DISABLED)
            self.imext_install_status_text.set("{} - {}".format(self.imext_install_status_text.get(), self._("Incompatible")))
            last_text_color = "red"
        self.is_checking_imext_install_status = False
        self.imext_install_status_label.configure(foreground=last_text_color)
        self.update_progress_text("")
        self.update_rwr_install_path_button.configure(state=tk.NORMAL)
    
    def update_progress_text(self, text, color="black"):
        if not self.progress_label.grid_info():
            self.progress_label.grid(column=0, columnspan=2, row=4, sticky="w", padx=(5, 10))
        
        if not text:
            self.progress_label.grid_forget()
            self.root.geometry("")
        
        self.progress_text.set(text)
        self.progress_label.configure(foreground=color)
    
    def update_md5_check_progress(self, progress_num, total_num):
        self.update_progress_text("{}: {}/{}".format(
            self._("MD5 Checking Progress"),
            progress_num,
            total_num
        ))
    
    def update_md5_check_result_tree(self, same_file_dict:dict, orig_file_dict:dict, diff_file_dict:dict, miss_file_dict:dict):
        if miss_file_dict:
            miss_file_root_node = self.file_tree.insert("", "end", text=self._("Missing files"), open=True, tags=["MissingFiles"])
            for category in miss_file_dict.keys():
                local_category_text = self._(category)
                category_node = self.file_tree.insert(miss_file_root_node, "end", text=local_category_text, open=False, tags=["MissingFiles"])
                self.insert_path_node(self.file_tree, category_node, miss_file_dict[category], ["MissingFiles"], 0)
        
        if diff_file_dict:
            diff_file_root_node = self.file_tree.insert("", "end", text=self._("Mismatched files"), open=True, tags=["DiffFiles"])
            for category in diff_file_dict.keys():
                local_category_text = self._(category)
                category_node = self.file_tree.insert(diff_file_root_node, "end", text=local_category_text, open=False, tags=["DiffFiles"])
                self.insert_path_node(self.file_tree, category_node, diff_file_dict[category], ["DiffFiles"], 0)
        
        if orig_file_dict:
            time_str = datetime.fromisoformat(self.imext_verifier.install_conf["timestamp"]).isoformat(timespec="seconds")
            orig_file_root_node = self.file_tree.insert("", "end", text="{} ({})".format(
                self._("Matched game original files"), time_str
            ), open=False, tags=["OrigFiles"])
            for category in orig_file_dict.keys():
                local_category_text = self._(category)
                category_node = self.file_tree.insert(orig_file_root_node, "end", text=local_category_text, open=False, tags=["OrigFiles"])
                self.insert_path_node(self.file_tree, category_node, orig_file_dict[category], ["OrigFiles"], 0)
        
        if same_file_dict:
            same_file_root_node = self.file_tree.insert("", "end", text="{} ({})".format(
                self._("Matched IMExt files"), self.imext_verifier.install_conf["version"]
            ), open=False, tags=["SameFiles"])
            for category in same_file_dict.keys():
                local_category_text = self._(category)
                category_node = self.file_tree.insert(same_file_root_node, "end", text=local_category_text, open=False, tags=["SameFiles"])
                self.insert_path_node(self.file_tree, category_node, same_file_dict[category], ["SameFiles"], 0)

        if not self.file_tree_container.grid_info():
            self.file_tree_container.grid(column=0, columnspan=2, row=3, sticky="nsew", padx=(10, 10))
    
    def insert_path_node(self, file_tree:ttk.Treeview, parent_node, file_rel_path_dict:dict, tags:list[str], open_level_count=0, overwrite_text=None):
        sub_file_rel_path_dict = {}
        for file_rel_path in file_rel_path_dict.keys():
            file_rel_path_obj = Path(file_rel_path)
            if len(file_rel_path_obj.parts) > 1:
                if file_rel_path_obj.parts[0] not in sub_file_rel_path_dict.keys():
                    sub_file_rel_path_dict[file_rel_path_obj.parts[0]] = {
                        str(Path(*file_rel_path_obj.parts[1:])): file_rel_path_dict[file_rel_path]
                    }
                else:
                    sub_file_rel_path_dict[file_rel_path_obj.parts[0]][str(Path(*file_rel_path_obj.parts[1:]))] = file_rel_path_dict[file_rel_path]
            else:
                sub_file_rel_path_dict[file_rel_path] = file_rel_path_dict[file_rel_path]
        
        for first_part in sub_file_rel_path_dict.keys():
            if isinstance(sub_file_rel_path_dict[first_part], dict):
                node = file_tree.insert(parent_node, "end", text=first_part, open=open_level_count > 0, tags=tags)
                self.set_tree_node_path_value(file_tree, node, parent_node, first_part)
                self.insert_path_node(file_tree, node, sub_file_rel_path_dict[first_part], tags, open_level_count-1, overwrite_text)
            else:
                if overwrite_text != None:
                    if overwrite_text == "":
                        text = first_part
                    else:
                        text = "{} - {}".format(first_part, overwrite_text)
                else:
                    if not sub_file_rel_path_dict[first_part]:
                        text = first_part
                    else:
                        text = "{} - {}".format(first_part, self._(sub_file_rel_path_dict[first_part]))
                node = file_tree.insert(
                    parent_node, "end",
                    text=text,
                    open=open_level_count > 0, tags=tags
                )
                self.set_tree_node_path_value(file_tree, node, parent_node, first_part)
    
    def set_tree_node_path_value(self, file_tree:ttk.Treeview, node, parent, node_part):
        parent_game_file_path = file_tree.set(parent, "game_file_path")
        parent_imext_file_path = file_tree.set(parent, "imext_file_path")
        if parent_game_file_path:
            file_tree.set(node, "game_file_path", os.path.normpath(os.path.join(parent_game_file_path, node_part)))
        else:
            file_tree.set(node, "game_file_path", os.path.normpath(os.path.join(self.rwr_install_path.get(), node_part)))
        if parent_imext_file_path:
            file_tree.set(node, "imext_file_path", os.path.normpath(os.path.join(parent_imext_file_path, node_part)))
        else:
            file_tree.set(node, "imext_file_path", os.path.normpath(os.path.join(self.imext_path, node_part)))
    
    def show_file_context_menu(self, event:tk.Event, file_tree:ttk.Treeview):
        item_id = file_tree.identify_row(event.y)
        if item_id:
            menu = tk.Menu(self.root, tearoff=0)
            show_menu = False
            if file_tree.get_children(item_id):
                menu.add_command(label=self._("Expand All"), command=lambda: self.toggle_node(file_tree, item_id, True))
                menu.add_command(label=self._("Collapse All"), command=lambda: self.toggle_node(file_tree, item_id, False))
                show_menu = True
            file_tree.selection_set(item_id)
            game_file_path = file_tree.set(item_id, "game_file_path")
            imext_file_path = file_tree.set(item_id, "imext_file_path")
            if game_file_path or imext_file_path:
                if os.path.exists(os.path.join(game_file_path, "..")):
                    menu.add_command(label=self._("Open Game file in explorer"), command=lambda: self.open_in_explorer(game_file_path))
                    show_menu = True
                if os.path.exists(os.path.join(imext_file_path, "..")):
                    menu.add_command(label=self._("Open IMExt file in explorer"), command=lambda: self.open_in_explorer(imext_file_path))
                    show_menu = True
                if os.path.exists(game_file_path):
                    menu.add_command(label=self._("Open Game file"), command=lambda: os.startfile(game_file_path))
                    show_menu = True
                if os.path.exists(imext_file_path):
                    menu.add_command(label=self._("Open IMExt file"), command=lambda: os.startfile(imext_file_path))
                    show_menu = True
            if show_menu:
                menu.post(event.x_root, event.y_root)
    
    def open_in_explorer(self, file_path):
        subprocess.run("explorer /select, \"{}\"".format(os.path.normpath(file_path)))
    
    def toggle_node(self, tree:ttk.Treeview, node, open_state):
        tree.item(node, open=open_state)
        for child in tree.get_children(node):
            self.toggle_node(tree, child, open_state)
    
    def start_install_imext(self):
        # 有差异文件文件提示（imext资源判断版本是否大于或等于）
        # 备份文件夹
        # 替换插件文件
        # 替换字体
        # 替换本地化文件
        # 检查游戏设置(DX9和覆盖层)
        if not self.is_installing_imext:
            self.is_installing_imext = True
            self.update_rwr_install_path_button.configure(state=tk.DISABLED)
            self.update_backup_path_button.configure(state=tk.DISABLED)
            self.imext_install_button.configure(state=tk.DISABLED)
            self.imext_install_status_text.set(self._("Installing..."))
            self.imext_install_status_label.configure(foreground="black")
            self.update_progress_text(self._("Prepare for installation"))
            if self.install_imext_thread:
                self.install_imext_thread.join()
            self.install_imext_thread = threading.Thread(target=self.install_imext)
            self.install_imext_thread.daemon = True
            self.install_imext_thread.start()
    
    def install_imext(self):
        self.update_progress_text(self._("Prepare for backup"))
        backup_folder_path = os.path.normpath(os.path.join(
            self.backup_path.get(),
            datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        ))
        
        backup_msg_box = CustomMessageboxYesNo(
            self,
            title=self._("Backup"),
            message="{}\n{}".format(
                self._("Do you want to create backup in:"),
                backup_folder_path
            )
        )
        if backup_msg_box.result:
            backup_game_file_list = []
            for category in self.imext_verifier.install_conf["imext_file_md5_sorted"].keys():
                if category == "IMExt Resources":
                    continue
                else:
                    backup_game_file_list.extend(self.imext_verifier.install_conf["imext_file_md5_sorted"][category].keys())
            
            self.copy_files_by_file_rel_path_list(
                self.rwr_install_path.get(),
                backup_folder_path,
                backup_game_file_list
            )
        
        self.update_progress_text("{} 1/3".format(self._("Install IMExt")))
        imext_files = {}
        imext_files.update(self.imext_verifier.install_conf["imext_file_md5_sorted"]["IMExt Resources"])
        imext_files.update(self.imext_verifier.install_conf["imext_file_md5_sorted"]["Game Exe"])
        install_imext_files_msg_box = CustomMessageboxYesNo(
            self,
            title="{} 1/3".format(self._("Install IMExt")),
            message="{}\n {}".format(
                self._("Install imext files to game folder:"),
                self.rwr_install_path.get()
            ),
            file_rel_path_dict=imext_files
        )
        if install_imext_files_msg_box.result:
            self.copy_files_by_file_rel_path_list(
                self.imext_path,
                self.rwr_install_path.get(),
                list(imext_files.keys())
            )
        
        self.update_progress_text("{} 2/3".format(self._("Install IMExt")))
        install_imext_fonts_msg_box = CustomMessageboxYesNo(
            self,
            title="{} 2/3".format(self._("Install IMExt")),
            message="{}\n {}".format(
                self._("Install imext fonts to game folder:"),
                self.rwr_install_path.get()
            ),
            file_rel_path_dict=self.imext_verifier.install_conf["imext_file_md5_sorted"]["Game Fonts"]
        )
        if install_imext_fonts_msg_box.result:
            self.copy_files_by_file_rel_path_list(
                self.imext_path,
                self.rwr_install_path.get(),
                list(self.imext_verifier.install_conf["imext_file_md5_sorted"]["Game Fonts"].keys())
            )
        
        self.update_progress_text("{} 3/3".format(self._("Install IMExt")))
        install_imext_locale_msg_box = CustomMessageboxYesNo(
            self,
            title="{} 3/3".format(self._("Install IMExt")),
            message="{}\n {}\n {}".format(
                self._("Install utf-8 localization files to game folder:"),
                self.rwr_install_path.get(),
                self._("(For users with Chinese language settings, this step can be bypassed)")
            ),
            file_rel_path_dict=self.imext_verifier.install_conf["imext_file_md5_sorted"]["Localization Files"]
        )
        if install_imext_locale_msg_box.result:
            self.copy_files_by_file_rel_path_list(
                self.imext_path,
                self.rwr_install_path.get(),
                list(self.imext_verifier.install_conf["imext_file_md5_sorted"]["Localization Files"].keys())
            )
        
        if self.rwr_config_manager.get_render_system().lower() == "directx":
            warning_dx9_msg_box = messagebox.showwarning(
                title=self._("Warning"),
                message=self._("You have enabled DX9 rendering mode, which may conflict with the Steam Overlay and cause the game to crash. It is suggested to switch to OpenGL mode, or try disabling Steam Overlay in the RWR's Steam properties.")
            )
        
        self.update_progress_text(self._("Installation finished"))
        self.file_tree.delete(*self.file_tree.get_children())
        self.file_tree_container.grid_forget()
        self.root.geometry("")
        self.imext_install_status_text.set(self._("Checking installation..."))
        self.check_imext_install_status()
        self.is_installing_imext = False
            
    def copy_files_by_file_rel_path_list(self, src_dir, dest_dir, file_rel_path_list):
        os.makedirs(dest_dir, exist_ok=True)
        for file_rel_path in file_rel_path_list:
            src_file = os.path.join(src_dir, file_rel_path)
            if os.path.exists(src_file):
                dest_file = os.path.join(dest_dir, file_rel_path)
                dest_subdir = os.path.dirname(dest_file)
                os.makedirs(dest_subdir, exist_ok=True)
                shutil.copy2(src_file, dest_file)
            self.update_progress_text("{}: {}".format(self._("Copying"), file_rel_path))

class CustomMessageboxYesNo:
    def __init__(self, app:RwrImextInstallerApp, title:str, message:str, file_rel_path_dict:dict=None):
        self.app = app
        self._ = global_translation
        self.result = None
        self.top = tk.Toplevel(self.app.root)
        self.top.title(title)
        
        self.top.columnconfigure(0, weight=1)
        if file_rel_path_dict:
            self.top.rowconfigure(1, weight=1)
        else:
            self.top.rowconfigure(0, weight=1)
        
        self.message_frame = ttk.Frame(self.top)
        self.message_frame.columnconfigure(1, weight=1)
        self.message_icon_label = tk.Label(self.message_frame, bitmap="question", fg="blue") 
        self.message_icon_label.grid(column=0, row=0, padx=(10, 5), pady=(10, 5))
        self.message_text_label = ttk.Label(self.message_frame, text=message)
        self.message_text_label.grid(column=1, row=0, sticky="nsew", padx=(5, 10), pady=(10, 5))
        self.message_frame.grid(column=0, row=0, sticky="nsew")
        
        if file_rel_path_dict:
            self.file_tree_frame = ttk.Frame(self.top)
            self.file_tree_frame.columnconfigure(0, weight=1)
            self.file_tree_frame.rowconfigure(0, weight=1)
            self.file_tree = ttk.Treeview(self.file_tree_frame, columns=["game_file_path", "imext_file_path"], displaycolumns=(), height=5, show=["tree"])
            self.file_tree.bind("<Button-3>", lambda event: self.app.show_file_context_menu(event, self.file_tree))
            self.file_tree_scrollbar = ttk.Scrollbar(self.file_tree_frame, orient="vertical", command=self.file_tree.yview)
            self.file_tree.configure(yscrollcommand=self.file_tree_scrollbar.set)
            self.file_tree.grid(column=0, row=0, sticky="nsew", padx=(10, 0), pady=(5, 5))
            self.file_tree_scrollbar.grid(column=1, row=0, sticky="ns", padx=(0, 10), pady=(5, 5))
            self.app.insert_path_node(self.file_tree, "", file_rel_path_dict, [], 3, "")
            self.file_tree_frame.grid(column=0, row=1, sticky="nsew")
        
        self.button_frame = ttk.Frame(self.top)
        self.button_frame.columnconfigure(0, weight=1)
        self.button_frame.columnconfigure(1, weight=1)
        self.yes_button = ttk.Button(self.button_frame, text=self._("Yes"), command=self.on_yes)
        self.no_button = ttk.Button(self.button_frame, text=self._("No"), command=self.on_no)
        self.yes_button.grid(column=0, row=0, sticky="e", padx=(10, 5), pady=(5, 10))
        self.no_button.grid(column=1, row=0, sticky="w", padx=(10, 5), pady=(5, 10))
        if file_rel_path_dict:
            self.button_frame.grid(column=0, row=2, sticky="nsew")
        else:
            self.button_frame.grid(column=0, row=1, sticky="nsew")

        self.top.grab_set()  # 锁定主界面
        self.app.root.wait_window(self.top) # 等待此窗口关闭后再继续主程序

    def on_yes(self):
        self.result = True
        self.top.destroy()

    def on_no(self):
        self.result = False
        self.top.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = RwrImextInstallerApp(root)
    root.mainloop()
