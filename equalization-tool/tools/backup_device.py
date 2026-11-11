"""Back up all 4 MiB with per-block device MD5 before changing firmware."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import esptool

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise SystemExit('Refusing to overwrite an existing backup')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    rom = esptool.detect_chip(args.port, baud=115200)
    if rom.CHIP_NAME != 'ESP32-C3':
        rom._port.close()
        raise SystemExit('Expected ESP32-C3')
    stub = rom.run_stub()
    esptool.attach_flash(stub)
    data = bytearray()
    try:
        for address in range(0, 0x400000, 0x10000):
            stub.check_command('read flash', stub.ESP_CMDS['READ_FLASH'], struct.pack('<IIII', address, 0x10000, 1024, 1))
            block = bytearray()
            while len(block) < 0x10000:
                packet = stub.read()
                if len(packet) != 1024:
                    raise RuntimeError('Unexpected packet size')
                block.extend(packet)
                stub.write(struct.pack('<I', len(block)))
            if stub.read() != hashlib.md5(block).digest():
                raise RuntimeError(f'Device MD5 mismatch at {address:#x}')
            data.extend(block)
            if len(data) % 0x80000 == 0:
                print(f'BACKUP {len(data)}/{0x400000}', flush=True)
        args.output.write_bytes(data)
        meta = {'size':len(data), 'sha256':hashlib.sha256(data).hexdigest(), 'port':args.port,
                'verified':'device MD5 for each 64 KiB block', 'created_unix':time.time()}
        args.output.with_suffix('.json').write_text(json.dumps(meta, indent=2), encoding='utf-8')
        print(json.dumps(meta), flush=True)
        stub.hard_reset()
    finally:
        stub._port.close()

if __name__ == '__main__':
    main()
