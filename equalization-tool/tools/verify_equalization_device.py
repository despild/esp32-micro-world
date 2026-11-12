"""Real ESP32 checks with the sensor disconnected; no simulated ADC in live data."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import time
import serial
from PIL import Image
import zxingcpp

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'output/equalization-device'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', required=True)
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    result = {'time_utc': datetime.now(timezone.utc).isoformat(), 'sensor_connected': False}
    with serial.Serial(args.port, 115200, timeout=.1, write_timeout=2) as ser:
        ser.dtr = True
        ser.rts = False
        def request(command, key='device', timeout=10):
            ser.write((command + '\n').encode())
            end = time.monotonic() + timeout
            while time.monotonic() < end:
                try:
                    msg = json.loads(ser.readline())
                except (ValueError, UnicodeDecodeError):
                    continue
                if key in msg:
                    return msg
            raise RuntimeError(f'No response to {command}')

        def frame(name):
            raw = bytes.fromhex(request('FRAME', 'frame')['frame'])
            assert len(raw) == 360
            im = Image.new('L', (72, 40))
            im.putdata([255 if raw[y // 8 * 72 + x] & (1 << (y % 8)) else 0
                        for y in range(40) for x in range(72)])
            enlarged = im.resize((720, 400), Image.Resampling.NEAREST)
            enlarged.save(OUT / f'{name}.png')
            return enlarged

        result['status'] = request('STATUS')
        assert result['status']['firmware'] == 'equalization-1.0'
        assert result['status']['adc_task'] and result['status']['oled']
        assert not result['status']['online'], 'This check expects the sensor to be disconnected'
        assert result['status']['pressure'] is None
        try:
            assert request('ALONG')['screen'] == 0
            frame('graph-no-sensor')
            tests = request('SELFTEST', 'core_tests')
            assert tests['core_tests'] == 0, tests
            result['core_assertions'] = 20
            assert request('A')['screen'] == 1
            frame('menu')
            assert request('B')['screen'] == 2
            assert request('B')['screen'] == 3
            qr_wifi = zxingcpp.read_barcode(frame('wifi-qr'))
            assert qr_wifi and qr_wifi.text == 'WIFI:T:WPA;S:EQ-C3;P:eqtest32;;'
            assert request('A')['screen'] == 4
            qr_web = zxingcpp.read_barcode(frame('web-qr'))
            assert qr_web and qr_web.text == 'http://192.168.4.1'
            result['qr_framebuffer'] = 'PASS: Wi-Fi + web'
            request('ALONG')
            assert request('BLONG')['screen'] == 5
            zero = request('B')
            assert zero['result'] == 'sensor_error' and zero['screen'] == 11
            request('ALONG')
            # Open PHONE, choose WIFI OFF, then reopen PHONE to enable it again.
            for command in ['A', 'B', 'A', 'A']:
                request(command)
            off = request('B')
            assert off['ap'] is False and off['screen'] == 0
            request('A')
            on = request('B')
            assert on['ap'] is True
            request('ALONG')
            result['menu_and_wifi_toggle'] = 'PASS'
            result['http_loopback'] = []
            for endpoint in ['/', '/api/status', '/api/samples']:
                check = request('HTTPTEST ' + endpoint, 'http_selftest')
                assert check['http_selftest'], check
                result['http_loopback'].append(check)
            result['final_status'] = request('STATUS')
            assert result['final_status']['pressure'] is None
            assert result['final_status']['dropped'] == 0
        finally:
            ser.write(b'ALONG\n')
    image = ROOT / 'build/EqualizationTool/EqualizationTool.ino.bin'
    if image.exists():
        result['binary_bytes'] = image.stat().st_size
        result['binary_sha256'] = hashlib.sha256(image.read_bytes()).hexdigest()
        assert result['binary_bytes'] <= 1048576
    result['result'] = 'PASS'
    result['limits'] = [
        'Sensor and physical buttons not connected; ADC timing, actual pressure and wiring untested.',
        'QR decoded from OLED framebuffer; phone camera scan not verified.',
        'HTTP uses on-device TCP loopback; final phone over-air test remains separate.',
        'SELFTEST runs an isolated model, not a sensor emulator or live-graph data source.'
    ]
    (OUT / 'results.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    main()
