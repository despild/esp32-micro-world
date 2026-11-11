"""Send bounded diagnostic commands to the web-test firmware and collect JSON."""
import argparse
import json
from pathlib import Path
import time
import serial

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--port',required=True)
    p.add_argument('--command',action='append',default=[])
    p.add_argument('--seconds',type=float,default=8)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    allowed={'STATUS','AUTO','SCAN','HTTPTEST','WIFI','URL','FRAME'}
    allowed.update(f'POWER {n}' for n in range(2,20))
    allowed.update(f'CHANNEL {n}' for n in range(1,12))
    if any(command not in allowed for command in args.command):raise SystemExit('Unknown diagnostic command')
    if not 0<args.seconds<=30:raise SystemExit('Listen duration must be 0..30 seconds')
    lines=[]
    with serial.Serial(args.port,115200,timeout=.15,write_timeout=2) as ser:
        ser.dtr=True;ser.rts=False
        for command in args.command or ['STATUS']:
            ser.write((command+'\n').encode())
        until=time.monotonic()+args.seconds
        while time.monotonic()<until:
            line=ser.readline().decode(errors='replace').strip()
            if line:
                lines.append(line)
                print(line,flush=True)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps({'commands':args.command,'lines':lines},indent=2),encoding='utf-8')

if __name__=='__main__':main()
