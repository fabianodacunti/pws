# Bill of materials

Summary bill of materials for the complete station: items, quantities and substitutes. Suppliers and prices are deliberately left out, since they change over time and from country to country. Only off-the-shelf modules are used (CON-03).

## How to read this list

- Items are grouped by where they are installed, as in the [block diagram](block-diagram.svg).
- **ID:** item identifier (`B-xx`), used to refer to the item in the documentation and in the firmware.
- **Priority:** same as in the [requirements](../docs/requirements.md). E = Essential; D = Desirable (UV, particulate matter and lightning). Mounting items take the priority of what they hold.
- **PoC:** "yes" when the item is also used in the proof of concept (`poc/`).
- **To be confirmed:** technical data not yet checked against the datasheet.

## Electronics enclosure (IP65)

| ID | Item | Qty | Choice | Substitutes | Prio. | PoC | Notes |
|---|---|---|---|---|---|---|---|
| B-01 | Microcontroller board | 1 | ESP32-S3-DevKitC-1 N16R8 (16 MB flash, 8 MB PSRAM) | Another ESP32-S3 board with at least 8 MB of flash (e.g. N8R8) | E | yes | CON-01. The board's RGB LED is the status LED (FR-DIA-05); its GPIO depends on the board revision (to be confirmed). |
| B-02 | Real-time clock module | 1 | DS3231 | DS3231M | E | no | Common modules (ZS-042) include a charging circuit for a rechargeable cell; with a CR2032, remove the charging diode or resistor (to be confirmed). |
| B-03 | RTC backup battery | 1 | CR2032 | LIR2032 (if the charging circuit is kept) | E | no | Keeps the time during power outages (NFR-REL-02). |
| B-04 | Pressure module (barometer) | 1 | BMP390 | BMP280, BME280 | E | yes | Inside the enclosure, which needs a vent (B-12) so the inside pressure follows the outside. |
| B-05 | Lightning detector module | 1 | AS3935 | — | D | no | Interface: I2C or SPI, to be decided. Keep it away from noise sources (power supply, Wi-Fi antenna); recommended distance to be confirmed. |
| B-06 | microSD card module | 1 | 3.3 V SPI module | Module with regulator and level shifter, if powered from 5 V | E | no | Prefer a module without a 5 V level shifter, wired directly to 3.3 V. |
| B-07 | microSD card | 1 | 8 to 32 GB, high endurance | Any brand-name class 10 card | E | no | Estimate: ~1 kB per record (CSV + JSON Lines) × ~105,000 records per year (5 min interval) ≈ 100 MB/year; ~500 MB/year at the 1 min interval (NFR-RES-02). Actual record size to be confirmed. FAT32 limits the card to 32 GB. |
| B-08 | Factory reset button | 1 | The board's own BOOT button (B-01) | — | E | no | FR-CFG-09. No extra part; the enclosure must be opened, which is acceptable since the web page can also restore factory settings. BOOT (GPIO0) is a strapping pin: the firmware reads it only after startup, held down for a few seconds. |
| B-09 | Power supply | 1 | Encapsulated AC-DC module, 127/220 V to 5 V, at least 2 A (10 W), inside the enclosure | DIN-rail or enclosed 5 V supply | E | no | NFR-PWR-01. Peak consumption estimated below 1 A at 5 V (Wi-Fi + particulate matter sensor), to be confirmed. Prefer a safety-certified module. The supply warms the enclosure and generates noise: keep it away from the AS3935. |
| B-10 | Mains input protection | 1 set | Fuse holder with fuse, and terminal blocks for the 127/220 V input | — | E | no | Fuse rating according to the power supply (to be confirmed). All mains wiring is insulated and protected against touch inside the enclosure. |
| B-11 | IP65 enclosure | 1 | Plastic enclosure with lid, size to be defined with the mechanical design | IP65 electrical junction box | E | no | NFR-ENV-02. Opening on the top for the light and UV sensor window (B-41). |
| B-12 | Membrane vent | 1 | Vent plug with PTFE membrane, M12 thread | — | E | no | Equalizes pressure and reduces condensation while keeping the enclosure sealed (vent IP rating to be confirmed). |
| B-13 | Cable glands | 5 | PG7 or PG9, according to cable diameter | — | E | no | NFR-ENV-04. One per cable: power, wind (anemometer + wind vane), rain, radiation shield, particulate matter. |
| B-14 | RJ11 board connectors | 2 | RJ11 to terminal block adapter | Cut the plug and use terminal blocks | E | no | Commercial wind and rain sensors usually come with RJ11 plugs (to be confirmed for the chosen set). |
| B-15 | Wind and rain interface parts | 1 set | 10 kΩ resistors and 100 nF capacitors | — | E | no | Wind vane voltage divider, and pull-up + filter for the anemometer and rain gauge contacts (values to be confirmed against the sensor datasheet). |
| B-16 | Mounting plate | 1 | Perfboard or mounting plate, standoffs and screws | — | E | no | No custom printed circuit board (CON-03). |
| B-17 | Short I2C cables | 6 | Qwiic / STEMMA QT cables | Wires with Dupont connectors or terminal blocks | E | yes | Connect the I2C modules inside the enclosure. |
| B-18 | Silica gel packs | 2 | Desiccant packs | — | E | no | Reduce humidity inside the enclosure; replace during maintenance. |
| B-19 | Mains power cable | 1 | Outdoor-rated cable with plug, length to be defined with the installation | — | E | no | Enters through the power cable gland (B-13). A circuit with a breaker and a residual-current device is recommended (applicable standard to be confirmed). |

## Radiation shield

| ID | Item | Qty | Choice | Substitutes | Prio. | PoC | Notes |
|---|---|---|---|---|---|---|---|
| B-21 | Temperature and humidity module | 1 | SHT45 | SHT41, SHT40 | E | yes | A version with a factory-fitted PTFE filter protects against dust and water (availability as a module to be confirmed). |
| B-22 | Radiation shield | 1 | Off-the-shelf (stacked plates) | Homemade (plant pot saucers) or printed by a service (CON-07) | E | no | Defined with the mechanical design. |
| B-23 | Shield-to-enclosure cable | 1 | Shielded 4-wire cable, length to be defined | Network cable (twisted pair) | E | no | I2C is not meant for long cables (see B-24). |
| B-24 | I2C bus extender | 2 | Pair of extender modules (e.g. differential) | — | E | no | Only if the distance between the shield and the enclosure is long (to be decided with the mechanical design). One module at each end. |

## Mast

| ID | Item | Qty | Choice | Substitutes | Prio. | PoC | Notes |
|---|---|---|---|---|---|---|---|
| B-31 | Wind and rain sensors (anemometer, wind vane and rain gauge) | 1 set | Commercial set such as the SparkFun Weather Meter Kit | Misol equivalent | E | no | Anemometer and rain gauge are contact closures (pulses); the wind vane is a resistor network read by the ADC. Conversion factors and the wind vane table are configurable. |
| B-32 | Mast and brackets | 1 set | Tube, clamps and brackets, to be defined with the mechanical design | — | E | no | Height and position depend on the installation site. |
| B-33 | Surge protection | 1 set | To be defined | — | D | no | NFR-ENV-05: mast grounding and input protection (applicable recommendations to be confirmed). |

## Ventilated housing for the particulate matter sensor

| ID | Item | Qty | Choice | Substitutes | Prio. | PoC | Notes |
|---|---|---|---|---|---|---|---|
| B-36 | Particulate matter sensor | 1 | SPS30 or PMS5003 (to be decided) | The other one | D | no | Both are powered from 5 V (logic levels to be confirmed). SPS30: I2C or UART; PMS5003: UART. |
| B-37 | Ventilated housing | 1 | Box with rain-protected openings, to be defined with the mechanical design | — | D | no | Air must flow through; water must not get in. |
| B-38 | Particulate sensor cable | 1 | Short 4-wire cable | — | D | no | Same length concern as B-23 if I2C is used. |

## Enclosure top, under the window

| ID | Item | Qty | Choice | Substitutes | Prio. | PoC | Notes |
|---|---|---|---|---|---|---|---|
| B-41 | Light and UV sensor window | 1 | UV-transmitting material, to be defined | — | E | no | Common acrylic blocks much of the UV (to be confirmed); candidates: PTFE (diffuser) or quartz glass. |
| B-42 | Light module (lux meter) | 1 | VEML7700 | BH1750 (saturates in direct sunlight) | E | yes | Maximum lux at the lowest gain to be confirmed. |
| B-43 | UV module | 1 | LTR390 | — | D | yes | Substitutes still in production to be confirmed. |

## Not in this list

- Tools (soldering iron, multimeter, pliers) and bench items such as the USB programming cable.
- Parts for a future battery and solar powered version, which is out of scope.
