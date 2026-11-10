# Pocket World · ESP32-C3 mini 0.42″ OLED

[한국어](README.md) · [User guide](USER_GUIDE.en.md)

**Pets, a mini dungeon and the actual Doom engine run on an ESP32-C3 with a 72×40 monochrome OLED.** The Windows controller sends input and settings. Its optional live view enlarges framebuffer readback from the board.

## Quick start

Connect a board with this firmware and compatible game data installed, then double-click `Start-Pocket-World.bat`. It launches the ordinary .NET Framework application `bin/PocketWorld.exe` and finds the USB board automatically. Python is not required for the primary controller.

Use the **라이브 뷰 (Live view)** checkbox above the preview to enable or disable framebuffer transmission. OLED gameplay, keyboard input, status updates and Windows lock detection continue while the preview is off. The selection survives USB reconnection within the current app session; a new app session starts with live view on.

A fresh board requires firmware and separate Doom data. Follow the [user guide](USER_GUIDE.en.md).

## Features and status

| Feature | Status |
|---|---|
| Pet feeding, play, rest and autonomous life | Runs on the board |
| Original mini dungeon: enemies, keys and floors | Runs on the board |
| Silent original Doom, E1M1–E1M9 | Runs on the board; PC sends controls |
| Windows lock detection | Autonomous life on lock, selected game on unlock |
| Live view toggle | Available in controller 0.2.1 |
| Doom clean view | Wall boundaries, sprite/weapon outlines, quieter floors and reduced weapon bob |
| Arduboy Zelda-style adventure | [Separate preparation repository](https://github.com/despild/esp32c3-arduboyzelda); not included in current firmware |

The current dungeon button launches our mini game, not NES Zelda or an Arduboy game. The bottom-center Doom object is the player's weapon. Firing and weapon-change animations are retained. Small text and distant enemies remain difficult to distinguish at 72×40.

## Hardware

- ESP32-C3 at 160MHz, 4MB flash, no PSRAM.
- SSD1306 0.42″ OLED, 72×40, one bit per pixel; 360-byte output buffer.
- SDA GPIO5, SCL GPIO6, I²C address 0x3C, 400kHz.
- USB Serial/JTAG at 115200 baud. The test PC used COM4; your port may differ.

Check the pinout before flashing another board. See the [Doom module](https://github.com/despild/esp32c3-doom/blob/main/README.en.md) for flash layout and configuration.

## Layout

| Path | Contents |
|---|---|
| `controller/native/` | Primary C# WinForms source and application manifest |
| `controller/*.py` | Alternative Python/Tk controller for source execution |
| `firmware/PocketWorldHooks.h` | Original pet and mini-dungeon code |
| `modules/doom/` | Independent Git repository pinned as a submodule |
| `bin/` | Controller, firmware, build metadata and SHA256 hashes |
| `tools/` | Assembly, build and hardware verification tools |
| `verification/` | Real-board results and a controller screenshot |

`firmware/PocketWorld/` is generated, untracked build input. Edit the source Hook or Doom module instead.

## Build from source

Initialize the submodule from the repository root, then enter this board folder:

```powershell
git submodule update --init --recursive
cd esp32c3mini-oled0.42-72x40
./tools/build_native_controller.ps1
python -m venv .venv
.venv/Scripts/python.exe -m pip install -r requirements.txt
.venv/Scripts/python.exe tools/assemble_world.py
./modules/doom/tools/build.ps1 -Sketch "$PWD/firmware/PocketWorld"
```

Firmware builds require Arduino CLI and Espressif Arduino core **3.1.1**. Use `-ArduinoCli 'C:/path/arduino-cli.exe'` if the CLI is not installed at the default Arduino IDE location. Follow the user guide to prepare Doom data and flash.

## Verification and distribution

Real-board tests on 2026-11-10 checked loading all nine Doom maps, movement/fire, lock/unlock and input timeouts. Visibility changes were rechecked with turning, firing and E1M2/3/6/8. Controller 0.2.1 was checked for stopped frame packets while live view was off, continued game frames, and restored readback after re-enabling it. This is not a complete game playthrough. The automated WTS test injects notifications into the app's own window; it does not lock the PC.

The primary app uses ordinary .NET packaging instead of a Python bundler. It has no extracting launcher, global keyboard hook, network access or administrator request. It is unsigned, and acceptance by every antivirus product is not guaranteed. Keep security software enabled and record any detection name and file hash. SHA256 hashes are in [BUILD_INFO.json](bin/BUILD_INFO.json).

Integration and Doom-derived code are **GPL-2.0-or-later**. Retain `COPYING` and the [module notices](https://github.com/despild/esp32c3-doom/blob/main/NOTICE.md). Game WADs, original full-board backups and personal save data are excluded from distribution. The Arduboy repository documents its own separate licensing.
