"""Verify real board status, HTTP handler and both OLED QR framebuffers over USB."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import time
import serial
import zxingcpp
from PIL import Image

OUT = Path(__file__).resolve().parents[1] / 'output' / 'web-device'

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--port', required=True)
    args=p.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    results={'time_utc':datetime.now(timezone.utc).isoformat()}
    with serial.Serial(args.port,115200,timeout=.1,write_timeout=2) as ser:
        ser.dtr=True
        ser.rts=False
        def request(command,key,timeout=8):
            ser.write((command+'\n').encode())
            until=time.monotonic()+timeout
            while time.monotonic()<until:
                line=ser.readline()
                try: msg=json.loads(line)
                except (ValueError,UnicodeDecodeError): continue
                if key in msg:return msg
            raise RuntimeError(f'No {key} after {command}')
        try:
            results['status']=request('STATUS','device')
            assert results['status']['ap'] and results['status']['oled'],results['status']
            assert results['status']['ip']=='192.168.4.1',results['status']
            assert results['status']['ap_started'] and results['status']['hidden']==0,results['status']
            assert results['status']['config_error']==0,results['status']
            for command,expected in [('WIFI','WIFI:T:WPA;S:EQ-C3;P:eqtest32;;'),('URL','http://192.168.4.1')]:
                frame=bytes.fromhex(request(command,'frame')['frame'])
                assert len(frame)==360
                im=Image.new('L',(72,40))
                im.putdata([255 if frame[y//8*72+x]&(1<<(y%8)) else 0 for y in range(40) for x in range(72)])
                large=im.resize((720,400),Image.Resampling.NEAREST)
                im.save(OUT/f'{command.lower()}-native.png')
                large.save(OUT/f'{command.lower()}.png')
                decoded=zxingcpp.read_barcode(large)
                assert decoded and decoded.text==expected,(command,decoded)
                results[command]={'payload':decoded.text,'framebuffer_decode':'PASS'}
            results['http']=request('HTTPTEST','http_selftest')
            assert results['http']['http_selftest'],results['http']
        finally:
            ser.write(b'AUTO\n')
    results['limits']='Framebuffer and loopback HTTP verified; phone camera and over-air browser require user test.'
    (OUT/'results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
    print(json.dumps(results,indent=2))

if __name__=='__main__':
    main()
