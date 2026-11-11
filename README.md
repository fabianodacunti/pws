# PWS: Personal Weather Station

An open-source personal weather station built around an ESP32-S3, with off-the-shelf modules and firmware written in C with ESP-IDF and FreeRTOS.

> "PWS" is a provisional name.

## What it does

A single outdoor unit measures temperature, humidity, pressure, wind speed and direction, rain and light. UV, particulate matter and lightning are planned as optional measurements.

- No local display: data is available on a web page served by the station and over MQTT, with Home Assistant discovery.
- Uploads to Weather Underground and Windy.
- Records stored on an SD card; after a network outage, pending records are resent over MQTT.
- Accurate timestamps from a battery-backed real-time clock, synchronized over NTP. All data is in UTC.
- Wi-Fi only, mains powered.

The full list is in the [requirements](docs/requirements.md).

## Status

Early stage. Requirements, plan, block diagram and bill of materials are done. The proof of concept (I2C sensors on a development board) is next.

| Milestone | Content | Tag |
|---|---|---|
| MS1 | Proof of concept and planning | `v0.1.0` |
| MS2 | Architecture and I2C libraries | `v0.2.0` |
| MS3 | Wind, rain, particulate matter and lightning | `v0.3.0` |
| MS4 | Complete station on the bench | `v0.4.0` |
| MS5 | Station installed | `v0.5.0` |
| MS6 | Station validated | `v1.0.0` |

Details in the [plan](docs/plan.md).

## Repository layout

| Folder | Content |
|---|---|
| [`docs/`](docs/) | [Requirements](docs/requirements.md), [plan](docs/plan.md) and, later, test reports |
| [`hardware/`](hardware/) | [Block diagram](hardware/README.md), [bill of materials](hardware/bom.md) and, later, wiring |
| `poc/` | Proof of concept (coming next) |
| `firmware/` | Main firmware (later) |
| `mechanical/` | Enclosure, radiation shield and mounting (later) |

Each sensor will have its own library, in a separate repository, packaged as an ESP-IDF component.

## Branches

- `main`: released milestones only, each one tagged.
- `dev`: integration branch; work in progress lands here first.

## Licenses

| Part | License | File |
|---|---|---|
| Firmware and code | [MIT](https://opensource.org/license/mit) | [LICENSE](LICENSE) |
| Hardware (`hardware/`) | [CERN Open Hardware Licence v2, Permissive (CERN-OHL-P-2.0)](https://ohwr.org/cern_ohl_p_v2.txt) | [hardware/LICENSE](hardware/LICENSE) |
| Mechanical design (`mechanical/`) | [CERN-OHL-P-2.0](https://ohwr.org/cern_ohl_p_v2.txt) | Same as hardware |
| Documentation (`docs/`) | [Creative Commons Attribution 4.0 International (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/) | [docs/LICENSE](docs/LICENSE) |

In short: you may use, modify and share everything here, including in commercial and closed products, as long as you keep the copyright and license notices.

Copyright (c) 2026 Fabiano Bozzi D'Acunti.
