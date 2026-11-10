"""Non-destructive checks after the sword-save test and a hardware reset."""
import sys,json,argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
module=ROOT/'modules/arduboy'
if not module.exists():module=ROOT/'esp32c3-arduboyzelda'
sys.path.insert(0,str(module/'tools'))
import qa_board as qa
qa.OUT=ROOT/'output/arduboy-integration'
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--port',default='COM4');args=p.parse_args()
d=qa.Device(args.port)
try:
    d.run(1);d.send('LOCK 0');d.send('MODE 3');d.send('MIRROR 1');d.run(1)
    assert d.ard['sword']==1,'Sword was not restored from NVS'
    assert d.ard['rx']==1 and d.ard['ry']==2,'Expected overworld checkpoint'
    d.snapshot('save-restored')
    d.send('MODE 2');d.send('NEW 1');d.run(12)
    assert d.state['mode']==2 and d.state['map']==1 and d.state['frames']>5
    before=d.state['ammo'];d.run(2,2);d.run(1)
    assert d.state['ammo']<before,'Doom fire did not consume ammo'
    d.snapshot('doom-after-arduboy')
    d.send('MODE 1');d.run(2);assert d.state['mode']==1;d.snapshot('mini-dungeon')
    d.send('MODE 0');d.send('PET FEED');d.run(2);assert d.state['mode']==0;d.snapshot('pet')
    d.send('MODE 3');d.run(1);assert d.ard['sword']==1;d.snapshot('adventure-resumed')
    d.run(5,keepalive=False);assert d.state['auto'];d.snapshot('heartbeat-auto')
    d.run(1);assert not d.state['auto'];d.snapshot('complete')
    print('PASS: NVS after reboot, Doom fire, all four modes, heartbeat pause/resume')
finally:d.close()
