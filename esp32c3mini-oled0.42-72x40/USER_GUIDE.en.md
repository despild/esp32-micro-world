# Pocket World user guide

[한국어](USER_GUIDE.md) · [Project overview](README.en.md)

## 1. Launch

1. Connect the ESP32-C3 OLED board using a USB data cable.
2. Double-click `Start-Pocket-World.bat`, which launches `bin/PocketWorld.exe`.
3. Wait for `COM… 연결됨` (connected). If needed, select the port in the USB dropdown at the bottom right.
4. Choose **펫 (Pet) / 던전 (Dungeon) / DOOM**. Click the preview area to return keyboard focus to gameplay.

The primary app uses Windows .NET Framework. It needs no Python installation, installer or administrator privileges. Close Arduino Serial Monitor and other serial apps; only one program can own the port.

## 2. Toggle live view

Click **라이브 뷰 (Live view)** above the preview on the right.

- On: the board transmits its OLED framebuffer over USB for enlarged PC display.
- Off: the preview says it is off and gameplay continues on the OLED. Framebuffer transmission stops.
- Gameplay, arrows/buttons, mode selection, health/ammo updates and lock detection continue while off.
- Re-enabling displays fresh frames. The selection is reapplied after USB reconnection.
- Starting a new app session resets the default to on.

The current firmware already supports `MIRROR 0/1`; this controller feature does not require a firmware update.

## 3. Controls

| Input | Doom | Original mini dungeon |
|---|---|---|
| Up/Down or W/S | Forward/back | Up/down |
| Left/Right or A/D | Turn | Left/right |
| Ctrl or Z | Fire | Attack adjacent enemies; restart after death |
| Space | Use/open door/menu confirm | Use input |
| Q/E | Strafe | — |
| Space + Q/E | Change weapon | — |
| Enter or Esc | Menu | — |
| Tab | Automap | — |

Game keys are sent only while the controller window is active. Switching to another app releases held keys.

**Doom:** Select E1M1–E1M9 and click **새 게임 (New game)** to restart at that map. Audio is disabled. Saving stores a map-start checkpoint, not exact player positions and actor state.

**Dungeon:** Defeat enemies, collect the key and reach the exit to advance a floor. This is the project's mini dungeon, separate from the planned Arduboy port.

**Pet:** Use **먹이 (Feed), 놀이 (Play), 휴식 (Rest)**. Pet state, experience and best floor are stored in board NVS.

## 4. Windows lock and autonomous life

Keep the app running. On Windows lock, Doom pauses and the pet moves autonomously. In mini-dungeon mode the board explores automatically. Unlocking returns to the selected mode. Lock detection works while the controller is minimized.

**자동 생활 미리보기 (Preview autonomous life)** lets you test the transition without locking Windows. Press again to return to manual control. Closing the app or losing commands for about 3.5 seconds also starts autonomous behavior if the board remains powered. If PC sleep removes USB power, the board turns off.

## 5. Display settings

- **DOOM 가시성 향상 (Clean view):** Quieter floor/ceiling, emphasized wall boundaries and sprite/weapon outlines, reduced walking weapon bob.
- **OLED 밝기 (OLED brightness):** Adjusts the actual display contrast.
- **어두운 장면 보정 (Dark-scene correction):** Lifts darker scene values; default 50.
- **디더링 강도 (Dither strength):** Adjusts dot patterns representing shades; default 15. Lower it if the pattern is distracting.

## 6. Install or update firmware

Back up an existing board before replacing its firmware. Run these commands from this folder with the controller and serial monitor closed. Substitute your own port.

```powershell
python -m venv .venv
.venv/Scripts/python.exe -m pip install -r requirements.txt
.venv/Scripts/python.exe -m esptool --chip esp32c3 --port COM4 read-flash 0 0x400000 original-backup.bin
```

**Board with compatible Doom data already installed:** Update the supplied firmware, preserving WAD and NVS:

```powershell
.venv/Scripts/python.exe modules/doom/tools/flash.py --port COM4 --build bin/firmware --sketch PocketWorld --app-only
```

**Fresh board:** First follow the [Doom data preparation instructions](https://github.com/despild/esp32c3-doom/blob/main/README.en.md). An ordinary PC `doom1.wad` is not directly compatible; this engine requires GBA-preprocessed data.

```powershell
git clone https://github.com/Spritetm/esp32c3-doom-bauble modules/doom/downloads/upstream
git -C modules/doom/downloads/upstream checkout b7a005313dc0a1fa20b334efd4237a9fd7780413
.venv/Scripts/python.exe modules/doom/tools/prepare.py modules/doom/downloads/upstream --data-only
.venv/Scripts/python.exe modules/doom/tools/flash.py --port COM4 --build bin/firmware --sketch PocketWorld
```

Game-data use and redistribution rights are separate from the engine code. No WAD is included in this repository.

## 7. Build and verify

The README build commands produce `bin/PocketWorld.exe` and firmware in `modules/doom/build/firmware`. Flash the newly built firmware with:

```powershell
.venv/Scripts/python.exe modules/doom/tools/flash.py --port COM4 --sketch PocketWorld --app-only
```

Hardware tests manipulate the current game. Close the controller and save any progress you need before testing:

```powershell
.venv/Scripts/python.exe tools/device_smoke.py --port COM4 --all-maps
./bin/PocketWorld.exe --verify output/controller-verify.json
```

The first command restarts maps and exercises input/autonomous modes. The second toggles live view in the real app, checks WTS event handling, saves JSON/PNG results, and exits. It normally finishes in about 10 seconds with a working connection.

## 8. Troubleshooting

| Symptom | Check |
|---|---|
| Waiting for connection | USB data cable, port selection, other serial apps |
| Blank preview | Live view checkbox |
| No key response | Active controller window, preview focus, autonomous preview off |
| OLED not responding | SDA5/SCL6 pinout and display type |
| Doom data error | Compatible preprocessed WAD installed at 0x110000 |
| Antivirus warning | Record vendor, detection name and SHA256; inspect source/build metadata and report a suspected false positive to the vendor |

The app is unsigned. It does not hide detections or disable security software. A detection cause cannot be established without the actual warning details.
