"""Build the parent application around the independent GPL Doom module."""
from pathlib import Path
import shutil
ROOT=Path(__file__).resolve().parents[1]
module=ROOT/'modules/doom'
if not module.exists(): module=ROOT/'esp32c3-oled-doom'
source=module/'firmware/OledDoom'
dest=ROOT/'firmware/PocketWorld'
dest.mkdir(parents=True,exist_ok=True)
shutil.copytree(source/'src',dest/'src',dirs_exist_ok=True)
shutil.copy2(source/'partitions.csv',dest/'partitions.csv')
sketch=(source/'OledDoom.ino').read_text()
sketch=sketch.replace('static int selectedMode=2;','static int selectedMode=0;')
(dest/'PocketWorld.ino').write_text(sketch)
shutil.copy2(ROOT/'firmware/PocketWorldHooks.h',dest/'WorldHooks.h')
print(dest)
