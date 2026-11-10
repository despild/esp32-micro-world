# Pocket World 사용 가이드

[English](USER_GUIDE.en.md) · [프로젝트 소개](README.md)

## 1. 컨트롤러 실행

1. ESP32-C3 OLED 보드를 데이터 통신이 가능한 USB 케이블로 연결합니다.
2. `Start-Pocket-World.bat`를 실행합니다. 기본 프로그램은 `bin/PocketWorld.exe`입니다.
3. 오른쪽 위에 `COM… 연결됨`이 표시되면 사용할 수 있습니다. 자동 연결되지 않으면 오른쪽 패널을 아래로 스크롤한 뒤 USB 연결 목록에서 해당 COM 포트를 선택합니다.
4. **펫 / 던전 / DOOM / Arduboy · 포켓 퀘스트**을 선택합니다. 미리보기 영역을 클릭하면 게임 키보드 조작으로 포커스가 돌아옵니다.

기본 앱은 Windows .NET Framework를 사용합니다. Python·별도 설치 프로그램·관리자 권한이 필요하지 않습니다. COM 포트는 한 프로그램만 사용할 수 있으므로 Arduino 시리얼 모니터 등은 닫아주세요.

## 2. 라이브 뷰 켜기/끄기

미리보기 위 오른쪽의 **라이브 뷰** 체크박스를 누릅니다.

- 켜짐: 보드가 OLED 프레임을 USB로 전송하고 PC가 확대 표시합니다.
- 꺼짐: `라이브 뷰 꺼짐 · OLED에서 실행 중`을 표시합니다. 화면 전송만 중단합니다.
- 꺼진 상태에서도 게임, 방향키·버튼 입력, 모드 선택, 체력·탄약 상태, 잠금 감지는 계속 작동합니다.
- 다시 켜면 새 프레임부터 표시합니다. USB 재연결 때 현재 선택을 다시 적용합니다.
- 앱을 완전히 종료 후 다시 실행하면 기본값인 켜짐으로 시작합니다.

라이브 뷰는 `MIRROR 0/1`을 사용합니다. Arduboy 모드를 사용하려면 이 릴리스의 통합 펌웨어도 설치해야 합니다.

## 3. 조작

| 입력 | Doom | 자체 던전 |
|---|---|---|
| ↑ ↓ / W S | 전진·후진 | 위·아래 |
| ← → / A D | 회전 | 왼쪽·오른쪽 |
| Ctrl / Z | 발사 | 인접 적 공격, 사망 후 재시작 |
| Space | 문 열기·메뉴 확인 | 사용 입력 |
| Q / E | 옆걸음 | — |
| Space + Q / E | 무기 전환 | — |
| Enter / Esc | 메뉴 | — |
| Tab | 지도 | — |

컨트롤러 창이 활성화되어 있을 때만 게임 키를 전송합니다. 다른 앱으로 전환하면 누른 키를 해제합니다.

**Doom:** E1M1–E1M9를 고르고 **새 게임**을 누릅니다. 현재 플레이를 해당 맵의 시작 상태로 바꿉니다. 무음이며 저장 메뉴는 맵 시작 체크포인트를 저장합니다. 위치·적 상태 전체 저장은 지원하지 않습니다.

**던전:** 적을 물리치고 열쇠를 얻은 뒤 출구로 이동하면 다음 층으로 넘어갑니다. 이는 이 프로젝트의 미니 던전이며 Arduboy 포트와 별개입니다.

**Arduboy · 포켓 퀘스트:** 방향키/WASD 이동, Ctrl/Z 검·확인, Space 장착 아이템, Enter/Tab 장비·지도입니다. 시작 방에서 오른쪽 두 방의 상자 아래로 접근하면 검을 얻습니다. **모험 진행 저장**으로 아이템·방 진행을 저장합니다. 재부팅하면 지역 입구에서 이어집니다. **새 모험**은 이 게임의 저장만 초기화합니다. 전체 퍼즐·보스 완주는 아직 검증하지 않았습니다.

**펫:** 먹이·놀이·휴식 버튼을 사용합니다. 펫 상태·경험치·최고 도달 층은 보드 NVS에 저장됩니다.

## 4. Windows 잠금과 자동 생활

컨트롤러를 켜둔 채 Windows를 잠그면 Doom과 Arduboy 모험은 일시 정지하고 펫이 움직입니다. 자체 던전 모드에서는 자동 탐험합니다. 잠금 해제 시 선택한 모드로 돌아옵니다. 앱이 최소화되어 있어도 잠금 감지는 작동합니다.

**자동 생활 미리보기** 버튼으로 실제 잠금 없이 전환할 수 있습니다. 다시 누르면 수동 조작으로 돌아갑니다. 앱을 닫거나 USB 명령이 약 3.5초 동안 끊겨도 보드에 전원이 있으면 자동 생활로 전환합니다. PC 절전으로 USB 전원이 끊기면 보드도 꺼집니다.

## 5. 화면 설정

- **DOOM 가시성 향상:** 바닥·천장 패턴을 줄이고 벽 경계, 적과 무기 윤곽을 강조합니다. 무기의 걸음 흔들림을 억제합니다.
- **OLED 밝기:** 실제 OLED 대비를 조절합니다.
- **어두운 장면 보정:** 어두운 색을 흰 픽셀로 표현하는 기준을 보정합니다. 기본 50.
- **디더링 강도:** 점 패턴으로 명암을 표현하는 강도입니다. 기본 15. 점이 복잡하게 느껴지면 낮추세요.

## 6. 펌웨어 설치 또는 업데이트

다른 보드의 내용을 보존해야 한다면 먼저 전체 Flash를 백업하세요. 아래 명령은 이 폴더에서 실행하며, 컨트롤러와 시리얼 모니터는 닫아야 합니다. 포트는 자신의 PC에 맞게 바꿉니다.

```powershell
python -m venv .venv
.venv/Scripts/python.exe -m pip install -r requirements.txt
.venv/Scripts/python.exe -m esptool --chip esp32c3 --port COM4 read-flash 0 0x400000 original-backup.bin
```

**이 프로젝트의 호환 Doom 데이터가 이미 설치된 보드:** 제공된 펌웨어만 업데이트합니다. WAD와 NVS를 유지합니다.

```powershell
.venv/Scripts/python.exe modules/doom/tools/flash.py --port COM4 --build bin/firmware --sketch PocketWorld --app-only
```

**새 보드:** [Doom 데이터 준비](https://github.com/despild/esp32c3-doom/blob/main/README.md)를 먼저 수행합니다. 일반 PC용 `doom1.wad`는 그대로 사용할 수 없으며 GBA 형식으로 전처리된 데이터가 필요합니다.

```powershell
git clone https://github.com/Spritetm/esp32c3-doom-bauble modules/doom/downloads/upstream
git -C modules/doom/downloads/upstream checkout b7a005313dc0a1fa20b334efd4237a9fd7780413
.venv/Scripts/python.exe modules/doom/tools/prepare.py modules/doom/downloads/upstream --data-only
.venv/Scripts/python.exe modules/doom/tools/flash.py --port COM4 --build bin/firmware --sketch PocketWorld
```

게임 데이터의 사용·재배포 권리는 엔진 소스와 별도입니다. 이 저장소에는 WAD를 포함하지 않습니다.

## 7. 직접 빌드와 점검

README의 소스 빌드 명령을 사용하면 컨트롤러는 `bin/PocketWorld.exe`, 펌웨어는 `modules/doom/build/firmware`에 생성됩니다. 생성 펌웨어 업로드:

```powershell
.venv/Scripts/python.exe modules/doom/tools/flash.py --port COM4 --sketch PocketWorld --app-only
```

실제 보드 자동 검증은 현재 게임을 조작합니다. 테스트 전 앱을 닫고 필요한 진행 상태를 저장하세요.

```powershell
.venv/Scripts/python.exe tools/device_smoke.py --port COM4 --all-maps
./bin/PocketWorld.exe --verify output/controller-verify.json
```

첫 명령은 맵을 재시작하고 입력·자동 생활을 검사합니다. 두 번째는 실제 앱의 라이브 뷰를 껐다 켜고 WTS 이벤트 처리를 확인한 뒤 JSON·PNG를 저장하고 종료합니다. 약 10초 내에 완료되는 정상 연결을 전제로 합니다.

## 8. 문제 해결

| 증상 | 확인할 것 |
|---|---|
| 연결 대기 | 데이터 USB 케이블, COM 포트, 다른 시리얼 앱 종료 |
| 미리보기가 꺼져 있음 | ‘라이브 뷰’ 체크 상태 |
| 키가 반응하지 않음 | 컨트롤러 창 활성화, 미리보기 클릭, 자동 생활 해제 |
| OLED I²C 응답 없음 | 보드 핀 SDA5/SCL6 및 디스플레이 종류 |
| Doom 데이터 오류 | 호환 전처리 WAD와 0x110000 파티션 설치 여부 |
| 바이러스 경고 | 탐지명·백신 이름·파일 SHA256 기록, 소스/빌드 정보 확인 및 공급사 오탐 신고 |

현재 앱은 서명되지 않았습니다. 백신 탐지를 숨기거나 보안 프로그램을 끄는 기능은 없습니다. 경고 문구 없이는 탐지 원인을 확정할 수 없습니다.
