# 하드웨어 조사

조사 기준일: 2026-11-11. 제조사 제품 문서와 데이터시트를 우선 확인했습니다. 재고와 국내 도착 가격은 확정하지 않았으며 아래 품번은 구매 전 확인할 후보입니다.

## 센서에서 먼저 결정할 것

측정값은 `호스 내부 압력 − 주변 대기압`입니다. 차압센서의 P1에 호스를 연결하고 P2를 주변 대기에 통하게 하면 이 값을 직접 읽을 수 있습니다. 양압만 필요하면 단방향 게이지/차압 옵션, 흡입에 따른 음압까지 보고 싶으면 양방향 옵션을 선택합니다.

단위 관계는 `1 kPa = 10 hPa = 10 mbar`, `1 hPa = 1 mbar`입니다. ±500 Pa는 ±5 hPa에 불과합니다. 낮은 범위의 센서가 민감하더라도 필요한 범위에서 포화되면 사용할 수 없습니다.

초기 설계 후보는 **양압 0–20 kPa(0–200 hPa)**입니다. 이는 실제 사용 압력을 아직 측정하지 않은 상태에서 정한 개발상 범위이며 훈련 목표값이 아닙니다. 음압도 필요하면 ±10 kPa 후보의 양압 상한이 충분한지 확인하고, 부족하면 ±25 kPa 등으로 확대합니다. 지나치게 넓은 범위는 전체 범위 대비 오차 때문에 저압 정밀도에 불리할 수 있습니다.

## BMP180을 그대로 사용할 경우

BMP180은 절대압 센서이며 명시된 측정 범위는 300–1,100 hPa입니다. 따라서 시작 압력을 빼는 방식으로 상대압을 계산할 수 있지만, 예를 들어 주변이 1,013 hPa이면 양압 여유는 약 **87 hPa(8.7 kPa)**입니다. 0–200 hPa 상대압 설계를 이 부품의 명시 범위 안에서 구현할 수는 없습니다. [Bosch BMP180 데이터시트, Seeed 보관본](https://files.seeedstudio.com/wiki/Grove-Barometer_Sensor-BMP180/res/BMP180.pdf)

호스 포트도 없어 작은 밀폐 챔버, 케이블 관통부 밀봉, 호스 피팅을 추가해야 합니다. 이는 초기 화면/코드 실험에는 가능하지만 누설·수분·챔버 부피를 해결해야 하므로, 이번 호스형 기기에는 포트가 있는 센서를 우선 제안합니다. 센서 통기구에 접착제를 넣어 호스를 붙이는 방식은 피합니다.

## 후보 비교

| 후보 | 비교할 측정 범위 | 전기적 연결 | 판단 |
|---|---|---|---|
| **CFSensor XGZP6897D** | 0–20 kPa 또는 ±10 kPa 옵션 | 3.3 V 구동 가능, I²C, 두 포트 | 저비용 시제품 우선 후보. 옵션·세대 확인 필수 |
| **Honeywell ABP2** | ±10 / ±25 kPa 옵션 | 3.3 V·I²C·두 바브 포트 옵션 | 습기에 대응하는 액체 매체 옵션을 선택할 수 있는 대안 |
| **MPXV5010DP** | 0–10 kPa, 0–100 hPa | 5 V 전원, 아날로그 약 0.2–4.7 V | 호스 연결은 쉽지만 전압 변환·ADC 처리가 추가됨 |
| **MPXV7002DP** | ±2 kPa, ±20 hPa | 5 V 전원, 아날로그 약 0.5–4.5 V | 이번 범용 후보보다 범위가 좁음 |
| **Sensirion SDP810-500Pa** | ±0.5 kPa, ±5 hPa | 디지털 I²C, 호스형 | 정밀 저압 센서이나 제안 범위에 크게 못 미침 |

사양 출처: [CFSensor V3.3](https://cfsensor.com/wp-content/uploads/2026/09/XGZP6897D-Pressure-Sensor-V3.3.pdf), [Honeywell ABP2](https://automation.honeywell.com/us/en/products/sensing-solutions/sensors/pressure-sensors/board-mount-pressure-amplified/basic-abp2-series-board-mount-pressure-sensor), [MPXV5010DP](https://www.st.com/resource/en/datasheet/mpxv5010dp.pdf), [MPXV7002DP](https://www.st.com/resource/en/datasheet/mpxv7002dp.pdf), [SDP800/810](https://sensirion.com/file/datasheet_sdp800-d/).

### XGZP6897D를 주문할 때

제조사 V3.3 문서의 주문 예시는 다음과 같습니다. 문서 자체에서도 양산 여부를 주문 전에 확인하도록 안내하므로 재고가 확인된 확정 구매 목록은 아닙니다.

- `XGZP6897DC020KPDG`: 0–20 kPa, 양압.
- `XGZP6897DC010KPDPN`: −10–+10 kPa, 양방향.

확인한 V3.3은 공급전압 3.0–5.5 V, 통상 1.8 mA, 정상 모드 응답시간 통상 5 ms로 기재합니다. 3.3 V로 구동하고 I²C 풀업도 3.3 V에 연결하는 구성을 제안합니다. 최신 `DC` 예시는 주소 0x58과 해당 레지스터 규약을 사용하므로, 판매 중인 구형 모듈에 같은 드라이버를 그대로 적용한다고 가정하지 않습니다. **모듈 전체 품번과 동봉 데이터시트**를 확인해야 합니다. [제조사 V3.3, pp.2–8](https://cfsensor.com/wp-content/uploads/2026/09/XGZP6897D-Pressure-Sensor-V3.3.pdf)

21비트 출력은 21비트의 실제 정확도를 뜻하지 않습니다. 데이터시트의 총 오차는 해당 범위에 따라 ±2 또는 ±2.5 %FSS이므로 절대 압력값과 미세한 변화 감지 성능을 구분해야 합니다. 영점 조정만으로 기울기·온도 오차가 모두 없어지지는 않습니다. [같은 데이터시트 p.2](https://cfsensor.com/wp-content/uploads/2026/09/XGZP6897D-Pressure-Sensor-V3.3.pdf)

### Honeywell ABP2를 주문할 때

`ABP2`라는 계열명만으로 주문하면 안 됩니다. **3.3 V / I²C / differential / 두 바브 포트 / 원하는 범위 / 매체 옵션**을 함께 선택해야 합니다. 범위 코드는 `010KD` 또는 `100MD`가 ±10 kPa, `025KD` 또는 `250MD`가 ±25 kPa에 해당합니다.

액체 매체용 T/V 등의 옵션은 건조 가스용 N/D와 다릅니다. 데이터시트는 액체 매체 옵션의 내부 응축 습도 조건을 구별하며, 매체 적합성도 P1/P2에서 다릅니다. 호스는 적합성이 확인된 포트로 연결하고, 액체 옵션이 기기 전체의 방수나 세척 가능성을 의미한다고 해석하지 않습니다. [Honeywell ABP2 데이터시트, 환경·접액재료·주문표](https://prod-edam.honeywell.com/content/dam/honeywell-edam/sps/siot/en-us/products/sensors/pressure-sensors/board-mount-pressure-sensors/basic-abp2-series/documents/sps-siot-abp2-series-datasheet-32350268-en.pdf?download=false)

### 아날로그 센서를 선택할 경우

MPXV5010DP/7002DP는 위 표의 출력이 ESP32의 입력 허용전압을 넘을 수 있습니다. 분압·필터·ADC 입력 범위·전원 변동을 함께 설계해야 합니다. 외부 ADC를 붙이더라도 공급전압과 입력 최대치를 확인해야 하며, ADC의 설정 범위가 핀의 허용전압을 높여 주지는 않습니다. 이번 배터리 기기에는 I²C 방식이 연결을 단순하게 합니다. [MPXV5010DP 동작조건](https://www.st.com/resource/en/datasheet/mpxv5010dp.pdf), [ESP32-C3 데이터시트](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)

## 호스와 수분

권장 배치는 `교체형 접촉 팁 → 실리콘 호스 → 물방울 포집부 → 소수성 공기 필터 → P1`입니다. P2는 대기와 통하게 유지합니다. 호스 내경은 센서 포트 도면과 실제 끼움성을 보고 정하며, 모든 센서에 같은 규격이 맞는다고 가정하지 않습니다.

이 구성은 액체 유입을 줄이기 위한 설계안입니다. **소수성 필터는 수증기를 모두 제거하지 않으므로 센서 내부 응축까지 보장하지 않습니다.** XGZP6897D는 결로 환경을 피하도록 명시되어 있어 호흡의 습기와 온도차를 시험해야 합니다. 습기 신뢰성이 우선이면 ABP2의 적합한 매체 옵션을 우선 검토합니다. [CFSensor 환경 조건, p.10](https://cfsensor.com/wp-content/uploads/2026/09/XGZP6897D-Pressure-Sensor-V3.3.pdf), [Honeywell 매체 조건](https://prod-edam.honeywell.com/content/dam/honeywell-edam/sps/siot/en-us/products/sensors/pressure-sensors/board-mount-pressure-sensors/basic-abp2-series/documents/sps-siot-abp2-series-datasheet-32350268-en.pdf?download=false)

호스가 길거나 포집부·필터가 크면 응답과 사용감이 달라질 수 있습니다. 최종 호스·필터를 조립한 상태에서 누설 및 압력 변화 응답을 검증하고, 센서 포트에 호스의 당기는 힘이 직접 걸리지 않게 고정합니다.

## BLE와 보드 선택

기존 ESP32-C3에는 Bluetooth LE 기능이 있어 별도 블루투스 모듈을 추가할 필요가 없습니다. BLE GATT로 압력·시간·상태를 보내는 구성을 제안합니다. [Espressif ESP32-C3 데이터시트](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)

- Android: 전용 앱 또는 지원 브라우저의 Web Bluetooth로 구현 가능.
- iPhone: BLE 앱으로 구현 가능. 기본 Safari의 Web Bluetooth 지원을 전제로 웹앱을 설계하지 않음.
- Android와 iPhone을 함께 지원한다면 공통 BLE 규약을 먼저 정하고 Flutter 등으로 앱을 만드는 방안을 검토.

웹 방식의 제한은 BLE 하드웨어 지원 여부와 별개입니다. Safari에는 별도 브라우저/확장 대안이 있지만 기본 지원과 구분해야 합니다. [Web Bluetooth 구현 현황](https://github.com/whatwg/bluetooth/blob/main/implementation-status.md)

새 보드를 구입한다면 **Seeed XIAO ESP32C3**도 후보입니다. 크기 21×17.8 mm, 3.7 V 배터리 입력과 USB 충전을 제공하며 문서상 고속 충전전류는 380 mA입니다. 배터리의 허용 충전전류가 이를 감당해야 합니다. 보유 OLED 보드에도 동일한 충전회로가 있다고 가정하지 않습니다. [Seeed 공식 문서](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)

XIAO에서 배터리 선을 스위치로 끊으면 그 상태에서는 배터리 충전도 되지 않습니다. 전원 OFF 중 충전이 요구되면 보드 회로를 확인해 부하를 분리하거나, 아래의 외부 전원 분배 구조를 사용합니다.

## 충전식 전원

소형 휴대용으로는 **보호회로가 포함된 1셀 3.7 V 리튬이온/리튬폴리머 팩, 약 1,000 mAh**를 초기 후보로 제안합니다. 이는 용량 제안이며 사용시간 보증은 아닙니다. 얇은 케이스에는 파우치형이 편하고, 18650은 용량 확보에 유리하지만 홀더까지 부피가 커집니다.

기존 보드를 유지하고 OFF 중 충전도 지원하려면 **전원 분배(power-path/load-sharing)를 제공하는 BQ24074 모듈**이 적합한 후보입니다. USB로 기기를 구동하면서 배터리를 별도로 충전하고, USB가 없으면 배터리에서 공급하는 방식입니다. [TI BQ24074](https://www.ti.com/product/BQ24074)

예를 들어 Adafruit #4755는 USB-C와 LOAD 출력을 제공합니다. LOAD는 USB 입력 시 약 4.4 V이고 배터리 사용 시 배터리 전압을 따라가므로, 3.3 V 또는 5 V 고정 출력이 아닙니다. 기본 최대 충전전류는 1 A로 안내되므로 작은 배터리에 그대로 사용하지 말고 배터리 허용치에 맞춰 설정해야 합니다. [Adafruit #4755 제품 문서](https://www.adafruit.com/product/4755)

일반 TP4056 충전 모듈은 보드에 별도 전원 분배 회로가 있는지 확인해야 합니다. 보호회로가 있다는 사실만으로 충전과 부하가 분리된 것은 아닙니다. 단순히 배터리와 기기를 병렬로 붙인 구성은 이번의 OFF/충전 동작을 위한 기본안으로 채택하지 않습니다.

배터리 완충 전압 4.2 V를 ESP32 3.3 V 핀에 직접 넣지 않습니다. 레귤레이터, USB 역급전 방지, 배터리 보호회로 및 커넥터 극성은 실제 보드/팩을 정한 뒤 배선도에서 확정합니다.

사용시간은 예를 들어 **배터리 쪽 실측 평균전류가 80 mA**라면 `1,000 mAh / 80 mA = 12.5시간`이 이상적인 계산입니다. 실제 사용시간은 유효 용량, 저전압 종료, 화면 밝기, 무선 동작에 따라 줄어들므로 현 단계에서 몇 시간이라고 확정하지 않습니다.
