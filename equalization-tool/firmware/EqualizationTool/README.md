# HX710B 이퀄툴 펌웨어

보유 ESP32-C3 + SSD1306 72×40용입니다. [부품 목록과 배선](../../wiring-and-parts.md), [센서 검토](../../hx710b-review.md)를 함께 확인하세요. GPIO4=OUT, GPIO3=SCK, GPIO0=버튼 A, GPIO1=버튼 B. 두 버튼은 GND로 연결하는 순간 NO 접점입니다.

## 화면과 메뉴

전원을 켜면 **최근 6초 그래프**가 기본입니다. 실제 압력 교정 전에는 RAW 그래프, 센서 미연결이면 `NO SENSOR / CONNECT HX710B`를 표시합니다. 세로축 kPa 범위는 2/5/10/20/40 중 선택합니다. RAW 상태에서는 자동 범위입니다. OLED의 한 가로 픽셀에 들어오는 최솟값/최댓값을 함께 그려 짧은 피크를 보존합니다.

| 조작 | 그래프 화면 | 메뉴/다른 화면 |
|---|---|---|
| A 짧게 | 메뉴 열기 | 다음 항목. QR에서는 Wi-Fi/웹 QR 전환 |
| B 짧게 | OLED 그래프 멈춤/재개 | 선택. QR에서는 폰 메뉴로 돌아가기 |
| A 길게, 0.8초 | 그래프 유지 | 그래프로 돌아가기, 진행 중 교정 취소 |
| B 길게, 0.8초 | 영점 확인 화면 | 상위 화면. 측정 중이면 취소 |

영점 확인과 기준압 확인 화면에서는 A=뒤로, B=측정 시작입니다. `MEASURING` 화면 B는 취소입니다. 결과 화면은 A/B로 그래프에 돌아갑니다. OLED 멈춤은 센서 취득이나 브라우저 그래프를 멈추지 않으며 메뉴 진입 시 해제됩니다.

```text
GRAPH (기본)
└─ A → MENU
   ├─ PHONE (선택하면 Wi-Fi 켜짐)
   │  ├─ WIFI QR  ─ A로 두 QR 전환, B로 상위 메뉴
   │  ├─ WEB QR
   │  ├─ WIFI OFF ─ Wi-Fi 종료 후 그래프
   │  └─ BACK
   ├─ ZERO ─ OPEN HOSE ─ B ─ 80 samples ─ RESULT
   ├─ CALIBRATE ─ 1 / 2 / 5 / 10 kPa ─ 적용 확인 ─ B ─ RESULT
   ├─ SCALE ─ 2 / 5 / 10 / 20 / 40 kPa (OLED만)
   ├─ INFO ─ 센서 / 교정 계수 / 영점 상태
   └─ GRAPH
```

첫 버전은 버튼을 연결하기 전에도 웹 시험이 가능하도록 **부팅 시 Wi-Fi ON**입니다. 단독 측정 시 `PHONE → WIFI OFF`로 끌 수 있습니다. 꺼도 센서와 OLED는 계속 동작합니다. 재부팅 시 Wi-Fi는 다시 켜지고 OLED 범위는 5 kPa로 시작합니다.

## 휴대폰 실시간 그래프

- Wi-Fi **EQ-C3**, 암호 **eqtest32**, 주소 **http://192.168.4.1**.
- OLED QR은 메뉴에서만 표시합니다. 폰 연결 여부로 기본 그래프가 바뀌지 않습니다.
- Android Chrome은 로컬 네트워크 권한을 허용하고 인터넷 없는 Wi-Fi 연결을 유지합니다.
- 외부 라이브러리/CDN/인터넷 없이 기기 안의 페이지가 동작합니다.
- 브라우저가 약 100 ms마다 샘플을 묶어 받아 최근 20초를 표시합니다. 장치 취득 목표 40 SPS와 HTTP 갱신 속도는 별개입니다.
- 현재값·그래프·최댓값·영점·기준압 교정·OLED 확인·표시 일시정지·표시 중인 최근 20초 CSV 저장을 지원합니다. 전체 세션 기록 기능은 아닙니다.
- 기기 버퍼 512 samples(40 SPS에서 약 12.8초), 응답당 최대 64 samples. 잠시 끊긴 경우 버퍼 안에서 이어 받고, 누락/포화/센서 오류는 선을 끊어 표시합니다. 장시간 무손실 기록을 보장하지 않습니다.
- 보드 재부팅/영점/교정 등으로 데이터 의미가 바뀌면 화면 기록도 초기화합니다.
- 최대 연결 기기 2대. 브라우저의 숨김 탭은 요청 간격을 늘립니다.

## 영점과 교정

`pressure_kPa = (raw - session_zero) × saved_gain`입니다. 공장 환산값을 임의로 넣지 않습니다.

1. 매 부팅·센서 재연결 후, 호스를 대기에 열고 `ZERO`를 실행합니다. 80개 유효값을 평균냅니다. 40 SPS에서 약 2초이며 지연/실패 시 5초 내 중단합니다.
2. 최초에는 알려진 일정한 양압을 기준 압력계로 확인하고 `CALIBRATE`를 실행합니다. OLED는 1/2/5/10 kPa, 웹은 0.2–40 kPa 입력이 가능합니다. 이 값은 **교정 입력 허용 범위**이며 전체 범위의 정확도 보장이 아닙니다.
3. ADC 포화, 센서 오류, 기준압과 영점의 차이 부족, 측정 구간의 큰 흔들림을 검출하면 실패로 표시합니다. 기준압 구간 RAW 변동 폭은 영점과의 차이 5% 이하를 요구합니다. 이는 정밀도 보증 시험이 아닌 입력 오류 검출입니다.
4. 성공한 gain만 NVS `eqtool`에 저장합니다. 영점은 저장하거나 자동 측정하지 않습니다. 재부팅 후 영점을 다시 맞춥니다. 저장 실패 시 `SAVE FAILED`/웹 오류를 표시합니다.
5. 무압 복귀와 여러 압력점으로 선형성·히스테리시스·누설을 확인합니다. 단일 기준점 교정만으로 전체 범위의 정확도를 보장하지 않습니다.

교정 전에는 RAW 또는 영점을 뺀 RAW Δ를 표시합니다. 음압/40 kPa 초과를 0으로 자르지 않고 범위 밖 표시를 냅니다. 포화/통신 실패값을 정상적인 0 kPa로 치환하지 않습니다. 센서가 끊기면 영점을 무효화합니다.

## 취득과 API

HX710B는 24비트 부호 있는 데이터를 읽고 **총 27 클럭**으로 압력 40 SPS를 선택합니다. 26클럭의 전원 차이 측정과 혼동하지 않습니다. SCK의 HIGH가 전원 차단 시간으로 늘어나지 않도록 짧은 임계 구역에서 펄스를 만들고 나머지는 LOW로 유지합니다. 별도 FreeRTOS 작업이 센서를 읽어 큐에 넣습니다. 첫 변환 이후 100 ms를 버리고 DOUT 미응답/LOW 고착을 검출합니다. 실제 40 SPS와 타이밍은 센서 연결 후 로직 분석기로 확인해야 합니다. [AVIA 데이터시트](https://www.hestore.eu/en/prod_getfile.php?id=16147)

| API | 동작 |
|---|---|
| `GET /api/status` | 센서/교정/작업/Wi-Fi 상태, 부팅 ID, revision |
| `GET /api/samples?after=0&boot=...&revision=...` | `status`, `samples`, `next`, `more`, `lost`. 새 부팅/새 revision은 버퍼부터 재동기화 |
| `POST /api/control` | URL-encoded `action=zero`, `action=calibrate&kpa=2`, `action=display` |

샘플은 `[sequence, device_ms, raw, pressure_kPa, flags]`. 사용할 수 없는 값은 JSON `null`입니다. flags는 비트 OR: 1=센서 없음, 2=포화, 4=미교정, 8=교정 수집 중, 16=명목 양압 범위 밖, 32=ADC 큐 누락. 장치 시간은 unsigned 32비트 ms이며 웹은 시간 wrap을 확장해서 표시합니다. 순번 wrap 시 revision을 바꿉니다. 빠진 구간은 추정 보간하지 않습니다.

초기 시제품의 공개된 로컬 AP 암호입니다. 인터넷으로 포트 포워딩하지 않습니다. 암호를 바꿀 때는 QR도 함께 다시 생성해야 합니다. BLE, 배터리 잔량 측정, 충전 제어는 이 버전에 없습니다. 물리 스위치와 충전은 전원 회로가 담당합니다.

## 빌드와 업로드

Arduino ESP32 core **3.1.1**, FQBN `esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashSize=4M`. Wire/WiFi/WebServer/DNSServer/Preferences는 코어 내장 라이브러리만 사용합니다. 별도 HX710B 라이브러리는 설치하지 않습니다.

```powershell
# 저장소 루트에서 실행. 필요 시 -ArduinoCli / -ArduinoData / -Python 지정
./equalization-tool/tools/build_firmware.ps1

# 기존 전체 4 MiB 백업과 .json 메타데이터가 있는 장치에 한해 app만 기록
python equalization-tool/tools/flash_web_test.py --port COM4 --backup /path/to/verified-backup.bin --build equalization-tool/build/EqualizationTool --sketch EqualizationTool
python equalization-tool/tools/verify_equalization_device.py --port COM4
```

기존 factory 앱 영역 **0x10000 / 1 MiB**를 유지합니다. 빌드 결과가 이 영역을 넘으면 도구가 업로드를 거절합니다. 전체 erase, 파티션 변경, bootloader/NVS/WAD 덮어쓰기는 하지 않습니다. Arduino IDE의 일반 업로드 대신 이 저장소의 확인 도구를 사용하세요. 새 보드에 설치할 때는 해당 보드에 맞는 파티션/부트로더 설치 절차가 별도로 필요합니다.

`web/index.html`이 웹 UI 원본이고 `tools/embed_web.py`가 gzip으로 압축한 `page.h`를 생성합니다. HTTP `Content-Encoding: gzip`으로 전달해 기존 앱 영역 안에 넣습니다. 원본 변경 후 다시 생성합니다. QR 비트맵은 동작 확인된 `WebQrTest`와 같습니다. 기존 `WebQrTest`는 연결 문제 진단용으로 보존했습니다.

USB 115200 명령: `STATUS`, `FRAME`, `A`, `B`, `ALONG`, `BLONG`, `HTTPTEST /`, `HTTPTEST /api/status`, `HTTPTEST /api/samples`, `SELFTEST`. A/B 명령은 물리 버튼과 같은 메뉴 처리 경로를 호출합니다. SELFTEST는 별도의 모델 객체에서 20개 결정적 검증을 실행하며 실제 데이터 스트림에 가상값을 넣지 않습니다. 결과 0은 실패 비트가 없다는 뜻입니다.

브라우저 검증: `PLAYWRIGHT_MODULE`에 Playwright 경로, 필요 시 `BROWSER_EXE`를 지정하고 `node equalization-tool/tools/test_web.cjs`. 모의 전송으로 오류·교정·재접속·CSV·반응형 화면을 검사하며 실제 압력센서 시험을 대신하지 않습니다.

실제 검증 결과는 [검증 기록](../../verification/equalization-firmware.json)에 기록합니다. 센서·버튼 물리 배선, 압력 정확도, 노이즈, 최종 배터리 회로, 카메라 QR 인식은 조립 후 확인해야 합니다.

2026-11-12 최종 앱 1,047,584 bytes / 1,048,576 bytes, 여유 992 bytes. 업로드 후 flash hash 일치를 확인했습니다. 실제 보드의 20개 모델/버튼 처리 검증, 메뉴·Wi-Fi ON/OFF, QR 프레임버퍼 디코딩, 3개 HTTP 경로의 전체 응답 전송을 확인했습니다. 웹은 모의 전송 테스트 13개를 통과했고 사용자가 최종 Android Chrome 그래프 화면을 확인했습니다. 이는 실제 센서 정확도 시험 결과가 아닙니다.

실제 OLED 프레임버퍼: [센서 대기 그래프](../../verification/equalization-images/graph-no-sensor.png), [메뉴](../../verification/equalization-images/menu.png). [웹 대기 화면](../../verification/equalization-images/web-no-sensor-mock.png)은 자동 브라우저 테스트의 모의 전송 스크린샷이며 휴대폰 캡처는 아닙니다.
