"""Exercise the real board and save its exact OLED readback for inspection."""
from pathlib import Path
import argparse, json, time
import serial
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'output/device-test'

class Device:
    def __init__(self,port):
        OUT.mkdir(parents=True,exist_ok=True)
        self.ser=serial.Serial(port,115200,timeout=.03,write_timeout=1)
        self.ser.dtr=True;self.ser.rts=False
        self.buffer=bytearray();self.frame=None;self.state={};self.hello=False
        self.log=(OUT/'serial.log').open('w',encoding='utf-8')
        self.states=[];self.invalid=0;self.frames=0
    def send(self,text):self.ser.write((text+'\n').encode())
    def run(self,seconds,keys=0,keepalive=True):
        start=time.monotonic();last=0
        while time.monotonic()-start<seconds:
            now=time.monotonic()
            if keepalive and now-last>.08:
                self.send(f'KEY {keys}' if self.hello else 'HELLO');last=now
            self.buffer.extend(self.ser.read(8192))
            while b'\n' in self.buffer:
                raw,_,self.buffer=self.buffer.partition(b'\n')
                line=raw.decode(errors='replace').strip()
                if not line:continue
                try:m=json.loads(line)
                except ValueError:
                    self.log.write(line+'\n');self.log.flush()
                    if 'Guru' in line or 'DOOM ERROR' in line: print(line,flush=True)
                    self.invalid+=1;continue
                if m.get('t')=='hello':self.hello=True
                elif m.get('t')=='frame':
                    try:b=bytes.fromhex(m['hex'])
                    except ValueError: self.invalid+=1;continue
                    if len(b)==360:self.frame=b;self.frames+=1
                    else:self.invalid+=1
                elif m.get('t')=='state':
                    self.state=m;self.states.append(m)
                    self.log.write(json.dumps(m)+'\n');self.log.flush()
                    if m.get('fault'):raise RuntimeError('Device fault: '+json.dumps(m))
        return dict(self.state)
    def snapshot(self,name):
        assert self.frame is not None,'No display readback'
        im=Image.new('1',(72,40))
        for y in range(40):
            for x in range(72):im.putpixel((x,y),255 if self.frame[y//8*72+x]&(1<<(y%8)) else 0)
        im.save(OUT/f'{name}-native.png')
        im.resize((720,400),Image.Resampling.NEAREST).save(OUT/f'{name}.png')
    def close(self):self.ser.close();self.log.close()

def main():
    p=argparse.ArgumentParser();p.add_argument('--port',default='COM4');p.add_argument('--all-maps',action='store_true');a=p.parse_args()
    d=Device(a.port);results={}
    try:
        d.run(2);assert d.hello,'No protocol handshake'
        d.send('LOCK 0');d.send('MODE 2');d.send('NEW 1')
        results['boot']=d.run(12);d.snapshot('doom-start');print('BOOT',results['boot'],flush=True)
        assert d.state.get('map')==1 and d.state.get('frames',0)>5,'Doom did not run'
        assert d.state.get('oled'),'OLED did not acknowledge I2C'
        f=d.frame;before=d.state.get('ammo')
        results['fire']=d.run(2,2);d.snapshot('doom-fire');d.run(1)
        assert d.state.get('ammo',50)<before,'Fire input did not consume ammo'
        d.run(.6,1<<7);d.run(1.2,1<<4);d.run(.2)
        assert f!=d.frame,'Movement did not change framebuffer'
        d.snapshot('doom-move')
        d.send('LOCK 1');d.run(1);paused=d.state['frames'];results['locked']=d.run(2);d.snapshot('auto-pet')
        assert d.state['auto'] and d.state['frames']==paused,'Lock did not pause Doom'
        d.send('LOCK 0');results['resumed']=d.run(2)
        assert d.state['frames']>paused and not d.state['auto'],'Unlock did not resume'
        if a.all_maps:
            results['maps']=[]
            for n in range(1,10):
                d.send(f'NEW {n}');state=d.run(5);d.snapshot(f'e1m{n}')
                assert state.get('map')==n, f'Map {n} failed: {state}'
                results['maps'].append(state);print('MAP',n,state,flush=True)
        d.send('MODE 1');d.send('LOCK 1');results['dungeon']=d.run(12);d.snapshot('dungeon')
        d.send('MODE 0');d.send('PET FEED');results['pet']=d.run(2);d.snapshot('pet')
        d.send('LOCK 0');d.send('MODE 2');d.send('NEW 1');d.run(3)
        results['frames_received']=d.frames;results['non_json_lines']=d.invalid
        (OUT/'results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
        print('PASS',json.dumps(results),flush=True)
    finally:d.close()

if __name__=='__main__':main()
