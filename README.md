# ESP32 Micro World

작은 OLED 보드에서 실행하는 게임과 펫, 그리고 PC 컨트롤러를 모은 프로젝트입니다.

Games, pets and PC controllers for tiny ESP32 OLED boards.

| Board / 보드 | Documentation / 문서 |
|---|---|
| ESP32-C3 mini · 0.42-inch OLED · 72×40 | [한국어](esp32c3mini-oled0.42-72x40/README.md) · [English](esp32c3mini-oled0.42-72x40/README.en.md) · [사용 가이드 / User guide](esp32c3mini-oled0.42-72x40/USER_GUIDE.md) |

```sh
git clone --recurse-submodules https://github.com/despild/esp32-micro-world.git
```

Separate modules / 독립 모듈:

- [esp32c3-doom](https://github.com/despild/esp32c3-doom): native, silent Doom / 보드 내부에서 실행하는 무음 Doom.
- [esp32c3-arduboyzelda](https://github.com/despild/esp32c3-arduboyzelda): playable Pocket Quest experimental port, original replacement artwork and saved progress / 플레이 가능한 Pocket Quest 실험판. 자체 에셋·저장 기능을 포함하며 통합 펌웨어에서도 실행됩니다.

The board folder includes source, a Windows controller and firmware binaries without Doom WAD data. See its documentation for hardware, licenses and build instructions.

보드 폴더에는 소스, Windows 컨트롤러, Doom WAD 데이터를 제외한 펌웨어가 들어 있습니다. 하드웨어·라이선스·빌드 방법은 해당 폴더 문서를 참고하세요.
