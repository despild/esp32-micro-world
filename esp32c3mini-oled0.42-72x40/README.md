# Pocket World · ESP32-C3 mini 0.42″ OLED

[English](README.en.md) · [사용 가이드](USER_GUIDE.md) · [English user guide](USER_GUIDE.en.md)

**ESP32-C3의 72×40 흑백 OLED에서 펫·미니 던전·원작 Doom을 실행하고, Windows 컨트롤러로 조작합니다.** 게임 계산과 화면 렌더링은 보드에서 수행합니다. PC 라이브 뷰는 보드 화면을 받아 확대합니다.

## 바로 실행

이 펌웨어와 게임 데이터가 설치된 보드를 USB로 연결하고 `Start-Pocket-World.bat`를 더블클릭하세요. Windows 기본 .NET Framework 컨트롤러 `bin/PocketWorld.exe`가 실행되어 보드를 자동으로 찾습니다. Python 설치는 기본 앱 실행에 필요하지 않습니다.

미리보기 위 **라이브 뷰** 체크박스로 PC 화면 전송을 켜고 끕니다. 끈 상태에서도 OLED, 게임 조작, 상태 표시와 잠금 감지는 계속 동작합니다. 같은 앱 세션에서 USB를 다시 연결해도 선택을 유지합니다. 앱을 새로 실행하면 켜짐으로 시작합니다.

새 보드는 펌웨어와 별도 Doom 데이터 설치가 필요합니다. [사용 가이드](USER_GUIDE.md)를 확인하세요.

## 기능과 현재 상태

| 기능 | 상태 |
|---|---|
| 펫 먹이·놀이·휴식, 자동 생활 | 보드에서 실행 |
| 자체 미니 던전, 적·열쇠·층 진행 | 보드에서 실행 |
| 원작 Doom 무음 E1M1–E1M9 | 보드에서 실행, PC는 입력만 전송 |
| Windows 잠금 감지 | 잠금 시 자동 생활, 해제 시 선택 게임 복귀 |
| 라이브 뷰 켜기/끄기 | 컨트롤러 0.2.1에서 지원 |
| Doom 가시성 향상 | 벽 경계·적/무기 윤곽, 바닥 패턴 감소, 무기 흔들림 억제 |
| Arduboy 젤다 스타일 모험 | [별도 저장소](https://github.com/despild/esp32c3-arduboyzelda)에서 준비 중, 현재 펌웨어에는 미포함 |

현재 ‘던전’ 버튼은 자체 미니 게임입니다. NES 젤다나 Arduboy 게임이 아닙니다. Doom 화면 아래 중앙 요소는 플레이어의 무기이며, 발사·교체 동작은 유지합니다. 72×40에서는 작은 글자와 먼 적의 식별에 한계가 있습니다.

## 하드웨어

- ESP32-C3, 160MHz, Flash 4MB, PSRAM 없음.
- SSD1306 0.42″, 72×40, 1비트. 프레임 버퍼 360바이트.
- SDA GPIO5 / SCL GPIO6 / I²C 0x3C, 400kHz.
- USB Serial/JTAG, 115200 baud. Windows에서 테스트한 포트는 COM4이며 PC마다 다를 수 있습니다.

핀 배치가 다른 보드에는 그대로 업로드하지 마세요. 설정과 Flash 배치는 [Doom 모듈](https://github.com/despild/esp32c3-doom/blob/main/README.md)을 참고하세요.

## 파일 구성

| 경로 | 내용 |
|---|---|
| `controller/native/` | 기본 C# WinForms 컨트롤러 소스와 manifest |
| `controller/*.py` | Python/Tk 소스 실행용 대체 컨트롤러 |
| `firmware/PocketWorldHooks.h` | 자체 펫·던전 로직 |
| `modules/doom/` | 별도 Git 저장소를 고정 revision으로 연결한 submodule |
| `bin/` | 컨트롤러와 펌웨어, SHA256·빌드 정보 |
| `tools/` | 조립·빌드 및 보드 검증 도구 |
| `verification/` | 실제 USB 보드 검증 결과와 UI 캡처 |

`firmware/PocketWorld/`는 조립된 빌드 입력이며 Git에 포함하지 않습니다. 수정은 원본 Hook 또는 Doom 모듈에서 하세요.

## 소스에서 빌드

저장소 루트에서 submodule을 가져온 뒤 이 보드 폴더로 이동합니다.

```powershell
git submodule update --init --recursive
cd esp32c3mini-oled0.42-72x40
./tools/build_native_controller.ps1
python -m venv .venv
.venv/Scripts/python.exe -m pip install -r requirements.txt
.venv/Scripts/python.exe tools/assemble_world.py
./modules/doom/tools/build.ps1 -Sketch "$PWD/firmware/PocketWorld"
```

펌웨어 빌드는 Arduino CLI와 Espressif Arduino core **3.1.1**이 필요합니다. CLI 경로가 기본 Arduino IDE 경로와 다르면 `-ArduinoCli 'C:/path/arduino-cli.exe'`를 지정하세요. Doom 데이터 준비와 업로드는 사용 가이드에 따릅니다.

## 검증과 배포

2026-11-10 실제 보드에서 Doom 9개 맵 로딩, 이동·발사, 잠금/해제, 입력 타임아웃을 확인했습니다. 가시성 변경 후 회전·발사와 E1M2/3/6/8을 재확인했습니다. 최신 컨트롤러는 라이브 뷰 OFF 시 화면 패킷 중단·게임 프레임 진행, ON 시 화면 복원을 확인했습니다. 전체 게임 완주 검증은 아닙니다. WTS UI 자동 검증은 앱 자체 창에 잠금/해제 알림을 주입한 테스트이며 실제 PC를 잠그지 않습니다.

기본 앱은 Python 번들러 대신 일반 .NET 실행 파일입니다. 압축 해제 런처, 전역 키보드 후킹, 네트워크 통신, 관리자 권한 요청을 사용하지 않습니다. 코드 서명은 되어 있지 않으며 백신 오탐이 절대 없음을 보증하지 않습니다. 보안 프로그램을 해제하지 말고 탐지명과 파일 해시를 확인하세요. [빌드 정보](bin/BUILD_INFO.json)에 SHA256을 제공합니다.

통합 코드와 Doom 파생 코드는 **GPL-2.0-or-later**입니다. `COPYING`과 [모듈 고지](https://github.com/despild/esp32c3-doom/blob/main/NOTICE.md)를 유지하세요. 게임 WAD·원본 보드 전체 백업·사용자 저장 데이터는 배포에 포함하지 않습니다. Arduboy 자료의 라이선스는 해당 저장소에서 별도로 설명합니다.
