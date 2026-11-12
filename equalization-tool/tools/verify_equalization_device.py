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
    report = OUT / 'results.json'
    report.write_text(json.dumps({**result, 'result': 'RUNNING'}, indent=2), encoding='utf-8')
    with serial.Serial(args.port, 115200, timeout=.1, write_timeout=2) as ser:
        ser.dtr = True
        ser.rts = False
        receive = bytearray()
        reply_lengths = set()
        def request(command, key='device', timeout=10):
            observed = []
            ser.write((command + '\n').encode())
            end = time.monotonic() + timeout
            while time.monotonic() < end:
                receive.extend(ser.read(ser.in_waiting or 1))
                while b'\n' in receive:
                    line, _, tail = receive.partition(b'\n')
                    receive[:] = tail
                    observed.append(line.decode('utf-8', errors='replace'))
                    try:
                        msg = json.loads(line)
                    except (ValueError, UnicodeDecodeError):
                        continue
                    if key in msg:
                        reply_lengths.add(len(line) + 1)  # Includes CRLF, not the extra blank delimiter.
                        return msg
            report.write_text(json.dumps({**result, 'result': 'FAIL', 'command': command,
                                         'observed_lines': observed,
                                         'partial_response': receive.decode('utf-8', errors='replace')}, indent=2), encoding='utf-8')
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
        assert result['status']['firmware'] == 'equalization-1.1'
        assert result['status']['adc_task'] and result['status']['oled']
        assert not result['status']['online'], 'This check expects the sensor to be disconnected'
        assert result['status']['demo'] is True
        assert result['status']['pressure'] is None and result['status']['raw'] is None
        try:
            assert request('ALONG')['screen'] == 0
            first = frame('graph-no-sensor')
            time.sleep(.7)
            assert first.tobytes() != frame('graph-demo-moving').tobytes(), 'Demo must scroll'
            request('B')
            frozen = frame('graph-demo-held')
            time.sleep(.7)
            assert frozen.tobytes() == frame('graph-demo-held-again').tobytes(), 'HOLD must freeze demo'
            request('B')
            resumed = frame('graph-demo-resumed')
            time.sleep(.7)
            assert resumed.tobytes() != frame('graph-demo-running').tobytes(), 'Demo must resume'
            result['oled_demo'] = 'PASS: scrolling, HOLD, resume; raw/pressure remain null'
            # STATUS and FRAME exceed the native USB driver's old 256-byte TX
            # buffer. Repeated complete replies catch truncation under host load.
            for _ in range(30):
                status = request('STATUS')
                assert status['demo'] and status['raw'] is None and status['pressure'] is None
            for _ in range(5):
                assert len(bytes.fromhex(request('FRAME', 'frame')['frame'])) == 360
            result['usb_replies'] = 'PASS: 30 complete STATUS + 5 complete FRAME replies'
            tests = request('SELFTEST', 'core_tests')
            assert tests['core_tests'] == 0, tests
            result['core_assertions'] = 25
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
            assert result['final_status']['raw'] is None and result['final_status']['demo'] is True
            assert result['final_status']['dropped'] == 0
            result['usb_reply_bytes'] = sorted(reply_lengths)
            result['usb_packet_boundary_replies'] = sorted(n for n in reply_lengths if n % 64 == 0)
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
