import os
import sys
script_folder_path = os.path.join(os.path.split(os.path.realpath(sys.argv[0]))[0], "python")
sys.path.append(script_folder_path)

from python.libs.imext_verifier import ImextVerifier, FILE_MASK

if __name__ == "__main__":
    iv = ImextVerifier("./")
    iv.create_install_conf_json(
        #version="1.0.1",
        #target_game_md5="b3ca1876364f3ae66e0a8d833e7b7199", # rwr_game.exe 2026-03-12
        file_mask=FILE_MASK,
        imext_folder_path="./RunningWithRifles",
        backup_folder_path="./backup/pre_build_game_update_backup"
    )