"""Build the parent application around the independent GPL Doom module."""
from pathlib import Path
import shutil,sys
ROOT=Path(__file__).resolve().parents[1]
module=ROOT/'modules/doom'
if not module.exists(): module=ROOT/'esp32c3-oled-doom'
source=module/'firmware/OledDoom'
adventure=ROOT/'modules/arduboy'
if not adventure.exists(): adventure=ROOT/'esp32c3-arduboyzelda'
dest=ROOT/'firmware/PocketWorld'
dest.mkdir(parents=True,exist_ok=True)
shutil.copytree(source/'src',dest/'src',dirs_exist_ok=True)
sys.path.insert(0,str(adventure/'tools'))
from stage_source import stage
stage(dest/'src/arduboy')
shutil.copy2(source/'partitions.csv',dest/'partitions.csv')
sketch=(source/'OledDoom.ino').read_text()
sketch=sketch.replace('static int selectedMode=2;','static int selectedMode=0;')
sketch=sketch.replace('value<=2) {selectedMode=value;', 'value<=3) {if(selectedMode==3&&value!=3) ardSave();selectedMode=value;')
sketch=sketch.replace('if(millis()-lastIdle>100)', 'if(millis()-lastIdle>(selectedMode==3&&!isAutonomous()?16:100))')
sketch=sketch.replace('if(engineStarted&&!failed) doom_get_stats(&hp,&ammo,&map);', 'if(engineStarted&&!failed) doom_get_stats(&hp,&ammo,&map); if(selectedMode==3){ArdStats s=ardStats();hp=s.hp;ammo=s.keys;map=s.dungeon;}')
sketch=sketch.replace('failed?"true":"false");', 'failed?"true":"false"); if(selectedMode==3) ardSendState();')
sketch=sketch.replace('\\"native\\":true}', '\\"native\\":true,\\"arduboy\\":true}')
sketch_path=dest/'PocketWorld.ino'
if not sketch_path.exists() or sketch_path.read_text()!=sketch:sketch_path.write_text(sketch)
shutil.copy2(ROOT/'firmware/PocketWorldHooks.h',dest/'WorldHooks.h')
print(dest)
