"""Check map/menu rendering, movement, and heartbeat loss on the actual board."""
import json
import device_smoke as test
test.OUT=test.ROOT/'output/device-controls'
d=test.Device('COM4')
try:
    d.run(1);d.send('LOCK 0');d.send('MODE 2');d.send('NEW 1');d.run(3)
    d.run(.15,1<<9);d.run(1);d.snapshot('automap')
    d.run(.15,1<<9);d.run(.3)
    d.run(.15,1<<8);d.run(1);d.snapshot('menu')
    d.run(.15,1<<8);d.run(.3)
    for keys in (16,128,18,16,65,32):d.run(2,keys)
    d.run(.2);d.snapshot('explore')
    d.run(5,keepalive=False)
    assert d.state.get('auto'),'Heartbeat timeout did not enter autonomous mode'
    d.snapshot('disconnected-auto')
    d.run(2)
    assert not d.state.get('auto') and d.state['frames']>50
    (test.OUT/'results.json').write_text(json.dumps(d.state,indent=2))
    print('PASS controls / menu / map / heartbeat timeout',flush=True)
finally:d.close()
