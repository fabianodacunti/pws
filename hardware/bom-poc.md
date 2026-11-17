# Bill of materials: proof of concept

What it takes to build the proof of concept (`poc/`): an ESP32-S3 board and four I2C sensor modules, read in a loop and shown in a terminal dashboard. It is a subset of the [bill of materials](bom.md) of the complete station, and uses the same item IDs (`B-xx`). As there, suppliers and prices are left out.

## Parts

| ID | Item | Qty | Choice | Accepted substitutes | Notes |
|---|---|---|---|---|---|
| B-01 | Microcontroller board | 1 | ESP32-S3-DevKitC-1 N16R8 | Another ESP32-S3 board with at least 8 MB of flash (e.g. N8R8) | GPIO8 and GPIO9 must be accessible, or the I2C pins changed in `menuconfig`. |
| B-21 | Temperature and humidity module | 1 | SHT45 | SHT41, SHT40 | I2C address 0x44. Other variants of the chip use 0x45 and 0x46, which the code does not handle. |
| B-04 | Pressure module | 1 | BMP390 | None | I2C address 0x76 or 0x77, depending on the SDO pin of the module (chosen in `menuconfig`). See "Substitutes" below. |
| B-42 | Light module | 1 | VEML7700 | None | I2C address 0x10 (fixed). See "Substitutes" below. |
| B-43 | UV module | 1 | LTR390 | None | I2C address 0x53 (fixed). Optional: it is turned off in `menuconfig`. |
| B-17 | I2C cables | 4 | Qwiic / STEMMA QT cables | Breadboard with jumper wires | One cable with male pins at one end to reach the board, and three module-to-module cables, if the modules have two connectors. |
| | USB cable | 1 | USB cable with data lines | | A charge-only cable cannot program the board. |
| | Pin headers and soldering iron | | | | Only if the modules come with loose pin headers. |

## Substitutes

Only the substitutes listed above work with the code of the proof of concept without changes:

- **SHT41 and SHT40** use the same command and the same conversion formulas as the SHT45. Only the accuracy changes.
- **BMP280 and BME280** are not supported: their registers and formulas are different. A BME280 answers with the same chip ID (0x60) as the BMP390, so the identity check of the driver would pass and the readings would be wrong, with no error.
- **BH1750** is not supported either, since it needs another driver, and it saturates in direct sunlight.

## Wiring

All modules share one I2C bus, powered from the 3.3 V pin of the board.

| ESP32-S3 board | Every module |
|---|---|
| 3V3 | Supply (VIN, VCC or 3V3) |
| GND | GND |
| GPIO8 (default) | SDA |
| GPIO9 (default) | SCL |

GPIO8 and GPIO9 are not strapping pins (those are GPIO0, GPIO3, GPIO45 and GPIO46). The pins can be changed in `menuconfig`.

## Checks before connecting

- **Supply voltage.** The chips accept at most 3.6 V (SHT45: 1.08 to 3.6 V; BMP390: 1.65 to 3.6 V; VEML7700: 2.5 to 3.6 V; LTR390: 1.7 to 3.6 V). Use the 3V3 pin of the board, and avoid modules that only work at 5 V.
- **I2C addresses.** Each module needs a different address. With the modules above, the scan shows 0x10 (VEML7700), 0x44 (SHT45), 0x53 (LTR390) and 0x77 or 0x76 (BMP390).
- **Pull-up resistors.** Most modules have their own, and with four modules they end up in parallel. According to the I2C specification, the total should stay above about 1 kΩ at 3.3 V, so that the line current does not exceed 3 mA. Four modules with 10 kΩ pull-ups give 2.5 kΩ, which is fine; four with 2.2 kΩ give about 550 Ω, which is too low. In that case, disable the pull-ups of some modules (many have a solder jumper for it).

## References

Datasheets and revisions used by the code:

- Sensirion, [SHT4x datasheet](https://sensirion.com/products/catalog/SHT45), version 7.3.
- Bosch Sensortec, [BMP390 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp390-ds002.pdf), BST-BMP390-DS002, revision 1.7.
- Vishay, [VEML7700 datasheet](https://www.vishay.com/docs/84286/veml7700.pdf), revision 1.8, and [application note](https://www.vishay.com/docs/84323/designingveml7700.pdf), document 84323.
- Lite-On, [LTR-390UV-01 datasheet](https://optoelectronics.liteon.com/upload/download/DS86-2015-0004/LTR-390UV-01_Final_%20DS_V1.4.PDF), version 1.4 (2021). Older editions give other values for the UV sensitivity (1810 and 2300 counts per UV index), so check the revision of any copy of this datasheet before using it.
- NXP, [I2C-bus specification and user manual](https://www.nxp.com/docs/en/user-guide/UM10204.pdf), UM10204, revision 7.0.
