"""Flash only the app after verifying a full backup and matching partition layout."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]

def partitions(data):
    result=[]
    for offset in range(0,len(data)-31,32):
        magic,kind,sub,address,size,name,flags=struct.unpack_from('<HBBII16sI',data,offset)
        if magic!=0x50aa:break
        result.append((kind,sub,address,size,name.rstrip(b'\0'),flags))
    return result

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--port',required=True)
    p.add_argument('--backup',required=True,type=Path)
    p.add_argument('--build',type=Path,default=ROOT/'build/WebQrTest')
    p.add_argument('--sketch',choices=['WebQrTest','EqualizationTool'],default='WebQrTest')
    args=p.parse_args()
    backup=args.backup.read_bytes()
    meta=json.loads(args.backup.with_suffix('.json').read_text(encoding='utf-8'))
    if len(backup)!=0x400000 or hashlib.sha256(backup).hexdigest()!=meta['sha256']:
        raise SystemExit('Full backup missing or checksum mismatch')
    if args.port != meta['port']:
        raise SystemExit('Backup was made from another port; confirm the target device first')
    image=args.build/f'{args.sketch}.ino.bin'
    layout=args.build/f'{args.sketch}.ino.partitions.bin'
    expected=partitions(backup[0x8000:0x9000])
    actual=partitions(layout.read_bytes())
    if actual!=expected:
        raise SystemExit(f'Partition layout differs from backed-up device: {actual!r} / {expected!r}')
    if not any(x[0]==0 and x[2:5]==(0x10000,0x100000,b'factory') for x in actual):
        raise SystemExit('Expected 1 MiB factory partition at 0x10000')
    if not 0<image.stat().st_size<=0x100000:
        raise SystemExit('Application exceeds the existing factory partition')
    print('Backup verified; writing app at 0x10000 only. NVS, bootloader, partitions and WAD retained.',flush=True)
    subprocess.run([sys.executable,'-m','esptool','--chip','esp32c3','--port',args.port,'--baud','115200',
                    'write-flash','0x10000',str(image)],check=True)

if __name__=='__main__':
    main()
