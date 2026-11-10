# PWS requirements

Requirements for the main version of the PWS (Personal Weather Station): a single outdoor unit based on an ESP32-S3, built from ready-made modules.

## How to read this document

- **Functional requirements (FR)** state *what* the station does: measure, compute, store, send, show.
- **Non-functional requirements (NFR)** state *how well*, *under which conditions* or *within which limits* it does it.
- **Constraints (CON)** are choices fixed in advance that limit the design.
- **Out of scope (OOS)** lists what the main version deliberately does not do.

Columns:

- **ID:** stable identifier. IDs are never renumbered; a removed requirement keeps its ID and is marked as removed.
- **Pri.:** priority. **E** = Essential (the main version is not done without it); **D** = Desirable (implemented if time allows; can be dropped without breaking the station).
- **Ver.:** verification method. **T** = test, **I** = inspection (code, document or assembly), **D** = demonstration, **A** = analysis (calculation or data comparison).

Values marked *(to be confirmed)* still need to be checked against primary sources (datasheets, WMO Guide No. 8, service documentation).

---

## 1. Functional requirements

### 1.1 Measurements (FR-MEA)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-MEA-01 | The station shall measure air temperature, in °C. | E | T |
| FR-MEA-02 | The station shall measure relative humidity, in %. | E | T |
| FR-MEA-03 | The station shall measure atmospheric pressure at the station altitude (station pressure), in hPa. | E | T |
| FR-MEA-04 | The station shall measure wind speed, in m/s. | E | T |
| FR-MEA-05 | The station shall measure the direction the wind blows from, in degrees (0° = north, clockwise), with the resolution of the wind vane (typically 16 positions, 22.5°, to be confirmed). | E | T |
| FR-MEA-06 | The station shall measure precipitation, in mm, by counting the tips of the rain gauge. | E | T |
| FR-MEA-07 | The station shall measure illuminance, in lux. | E | T |
| FR-MEA-08 | The station shall measure ultraviolet radiation and express it as UV index. | D | T |
| FR-MEA-09 | The station shall measure particulate matter concentration (PM1.0, PM2.5 and PM10), in µg/m³. | D | T |
| FR-MEA-10 | The station shall detect lightning strikes and estimate the distance to the storm, in km. | D | T |
| FR-MEA-11 | The station shall discard events classified by the lightning sensor as disturbers (electrical noise), without counting them as strikes. | D | T |

### 1.2 Derived quantities (FR-CAL)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-CAL-01 | The station shall compute the dew point, in °C, from temperature and humidity. | E | A |
| FR-CAL-02 | The station shall compute the pressure reduced to sea level, in hPa, using the configured station altitude (FR-CFG-05). | E | A |
| FR-CAL-03 | The station shall compute the apparent temperature (heat index or wind chill, depending on conditions; formulas and validity ranges to be confirmed). | D | A |
| FR-CAL-04 | The station shall compute the rain rate, in mm/h. | E | A |
| FR-CAL-05 | The station shall compute the daily rain total, reset at local midnight. | E | A |
| FR-CAL-06 | The station shall compute the rain total for the last hour and the last 24 hours. | D | A |

### 1.3 Sampling and aggregation (FR-SMP)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-SMP-01 | Sensor sampling shall be independent of the publishing interval. | E | I |
| FR-SMP-02 | Wind shall be sampled continuously, counting anemometer pulses without missing any. | E | T |
| FR-SMP-03 | Rain shall be recorded by interrupt, with the timestamp of each tip. | E | T |
| FR-SMP-04 | Temperature, humidity, pressure, light, UV and particulate matter shall be read periodically, at a configurable interval (default 10 s). | E | T |
| FR-SMP-05 | For each publishing interval, the station shall compute mean, minimum and maximum of temperature, humidity and pressure. | E | A |
| FR-SMP-06 | For each publishing interval, the station shall compute the mean wind speed and the gust (highest mean over a short window; 3 s according to WMO Guide No. 8, to be confirmed). | E | A |
| FR-SMP-07 | The mean wind direction shall be computed as a vector average (not an arithmetic one, which fails around north). | E | A |
| FR-SMP-08 | The station shall report the gust direction. | D | A |
| FR-SMP-09 | For each interval, the station shall report the rain amount in that interval. | E | A |
| FR-SMP-10 | The particulate matter sensor shall run in duty cycles (on only part of the time) to extend its lifetime (per manufacturer recommendation, to be confirmed). | D | T |
| FR-SMP-11 | The anemometer and rain gauge conversion factors and the wind vane lookup table shall be configurable. | E | T |
| FR-SMP-12 | For each interval, the station shall report the number of lightning strikes and the estimated distance of the closest one. | D | A |

### 1.4 Time (FR-TIM)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-TIM-01 | The station shall keep date and time in a battery-backed RTC. | E | T |
| FR-TIM-02 | The station shall set the RTC from NTP at startup and every 6 h. | E | T |
| FR-TIM-03 | Without network, the station shall keep using the RTC time. | E | T |
| FR-TIM-04 | All stored and transmitted timestamps shall be in UTC. | E | I |
| FR-TIM-05 | Each record timestamp shall mark the **end** of the interval it summarizes (the 12:05 record covers 12:00 to 12:05). | E | I |
| FR-TIM-06 | Publishing intervals shall be aligned to the clock (e.g. 12:00, 12:05, 12:10), not to the moment the station started. | E | T |
| FR-TIM-07 | If the time has never been set (RTC without battery and no network), records shall be flagged as "time not reliable". | E | T |

### 1.5 Data logging (FR-LOG)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-LOG-01 | The station shall write one record per publishing interval to the SD card. | E | T |
| FR-LOG-02 | The record shall have the same content and data format used for MQTT and for the web API. | E | I |
| FR-LOG-03 | Records shall be written to CSV files, one file per day (UTC date). | E | I |
| FR-LOG-04 | Each record shall contain the UTC timestamp, all quantities of the interval, and a validity indicator per quantity (missing or failed sensor = empty value). | E | I |
| FR-LOG-05 | When the card is full, the station shall delete the oldest files and keep logging (circular logging). | E | T |
| FR-LOG-06 | Without an SD card, the station shall keep measuring and publishing, and report the missing card in its status. | E | T |
| FR-LOG-07 | The station shall keep track of records not yet sent to MQTT, for later resending (FR-MQT-06). | E | T |
| FR-LOG-08 | The station shall also write a copy of the records as JSON Lines (one JSON object per line), with exactly the same content as the CSV and the same file layout. | D | I |

### 1.6 Network (FR-NET)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-NET-01 | The station shall connect to a configured Wi-Fi network. | E | D |
| FR-NET-02 | The station shall reconnect automatically when Wi-Fi drops. | E | T |
| FR-NET-03 | With no network configured, or if the configured network is not found for 5 min, the station shall start its own access point with a page to configure Wi-Fi, while still trying the configured network. | E | D |
| FR-NET-04 | The station shall be reachable on the local network by name (e.g. `pws.local`, via mDNS). | D | D |

### 1.7 MQTT and Home Assistant (FR-MQT)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-MQT-01 | The station shall publish each record to a configurable MQTT broker, as JSON. | E | D |
| FR-MQT-02 | The station shall publish its online/offline status using a last will message (LWT). | E | T |
| FR-MQT-03 | The station shall periodically publish diagnostic data (see FR-DIA). | D | D |
| FR-MQT-04 | The station shall publish Home Assistant discovery messages, so that its sensors appear without manual configuration. | E | D |
| FR-MQT-05 | Home Assistant discovery shall be enabled or disabled in the configuration. | D | T |
| FR-MQT-06 | When the connection is restored, the station shall resend pending records (FR-LOG-07) in chronological order, on a dedicated topic for late data, without changing the "current" values. | E | T |
| FR-MQT-07 | Topics shall use the prefix `pws/<station id>/`, and the topic structure shall be documented. | E | I |

### 1.8 External services (FR-EXT)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-EXT-01 | The station shall upload measurements to Weather Underground, with configurable station ID and key. | E | D |
| FR-EXT-02 | The station shall upload measurements to Windy, with a configurable key. | E | D |
| FR-EXT-03 | Each external service shall be enabled or disabled individually. | E | T |
| FR-EXT-04 | External services receive only current values; data from periods without network is not resent to them. | E | I |
| FR-EXT-05 | The station shall convert values to the units required by each service (e.g. Weather Underground uses imperial units; to be confirmed in each API documentation). | E | T |
| FR-EXT-06 | The upload interval shall respect the minimum accepted by each service (limits to be confirmed in each API documentation). | E | I |

### 1.9 Web page and API (FR-WEB)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-WEB-01 | The station shall serve a web page with the current values of all quantities. | E | D |
| FR-WEB-02 | The page shall show 24 h, 7-day and 30-day charts, built from the SD card records. | E | D |
| FR-WEB-03 | The page shall show times in the local time zone (configurable), while data stays in UTC. | E | T |
| FR-WEB-04 | The page shall show the daily minimums and maximums and the daily rain total. | E | D |
| FR-WEB-05 | The page shall show a wind rose or a wind direction indicator. | D | D |
| FR-WEB-06 | The page shall have a station status view (see FR-DIA). | E | D |
| FR-WEB-07 | The page shall have a configuration view (see FR-CFG). | E | D |
| FR-WEB-08 | The station shall provide a JSON API with current values, history by period, and status. | E | T |
| FR-WEB-09 | The page shall allow downloading the data files from the SD card. | D | D |
| FR-WEB-10 | The page shall refresh the current values automatically, without reloading, every 10 s. | E | D |

### 1.10 Configuration (FR-CFG)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-CFG-01 | All settings shall be stored in NVS and survive restarts and firmware updates. | E | T |
| FR-CFG-02 | It shall be possible to configure from the web page: Wi-Fi; MQTT broker (address, port, user, password); Weather Underground; Windy. | E | D |
| FR-CFG-03 | It shall be possible to set the publishing interval to 1, 2, 5, 10 or 15 min, with a default of 5 min. | E | T |
| FR-CFG-04 | It shall be possible to set the reading interval of the slow sensors (FR-SMP-04). | D | T |
| FR-CFG-05 | It shall be possible to set the station altitude, latitude and longitude. | E | T |
| FR-CFG-06 | It shall be possible to set the time zone used by the web page. | E | T |
| FR-CFG-07 | It shall be possible to set the wind and rain sensors factors and the wind vane alignment to north. | E | T |
| FR-CFG-08 | It shall be possible to apply a correction (offset) to each quantity, for adjustments after field validation. | D | T |
| FR-CFG-09 | It shall be possible to restore factory settings both with a physical button and from the web page. | E | T |
| FR-CFG-10 | It shall be possible to export and import the configuration (JSON file). | D | T |

### 1.11 Firmware update (FR-OTA)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-OTA-01 | The station shall accept firmware updates over the network (OTA), by uploading the file from the web page. | E | D |
| FR-OTA-02 | If the new firmware fails to start correctly, the station shall roll back to the previous version on its own. | E | T |
| FR-OTA-03 | The station shall also allow updating the web page files (LittleFS). | D | D |
| FR-OTA-04 | The firmware version shall be shown on the web page and published via MQTT. | E | I |

### 1.12 Status and diagnostics (FR-DIA)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| FR-DIA-01 | The station shall report the status of each sensor (ok, missing, failed). | E | T |
| FR-DIA-02 | A failure in one sensor shall not prevent the others from being read and published. | E | T |
| FR-DIA-03 | The station shall report: uptime, last reset reason, Wi-Fi signal strength, free SD card space, last time synchronization, and records pending upload. | E | D |
| FR-DIA-04 | The station shall output log messages on the serial port. | E | D |
| FR-DIA-05 | The station shall use the board's RGB LED as a status LED (the states shown will be defined in the firmware design). | D | D |

---

## 2. Non-functional requirements

### 2.1 Measurement accuracy (NFR-ACC)

Reference: each sensor's specification. The figures below are typical datasheet values and are all to be confirmed.

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| NFR-ACC-01 | The accuracy of each quantity shall be the one specified by the sensor manufacturer, under the datasheet conditions. Reference values to confirm: SHT45 ±0.1 °C and ±1 %RH; BMP390 ±0.5 hPa absolute; anemometer and rain gauge as specified by their manufacturer. | E | I |
| NFR-ACC-02 | Temperature and humidity shall be measured inside a ventilated radiation shield, away from heat sources in the station's own circuit. | E | I |
| NFR-ACC-03 | Field validation shall compare trends and mean differences against INMET, CEPAGRI and the Viracopos airport METAR (SBKP), documenting the differences found. | E | A |
| NFR-ACC-04 | Anemometer and wind vane installation shall follow, as far as possible, the height and obstacle clearance recommended by WMO Guide No. 8 (reference: 10 m above ground, to be confirmed), documenting the deviations of the actual site. | D | I |

### 2.2 Performance and timing (NFR-PRF)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| NFR-PRF-01 | Each interval record shall be published within 10 s after the end of the interval. | E | T |
| NFR-PRF-02 | The web page shall load within 3 s on the local network. | E | T |
| NFR-PRF-03 | The 30-day chart shall load within 10 s. | D | T |
| NFR-PRF-04 | Anemometer pulse counting shall handle wind speeds up to 50 m/s without missing pulses. | E | T |
| NFR-PRF-05 | The station shall serve at least 3 simultaneous browsers without delaying measurements. | D | T |

### 2.3 Reliability and availability (NFR-REL)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| NFR-REL-01 | Powered from the mains, the station shall run 24 hours a day for at least 30 consecutive days without hanging and without manual intervention (restart, reconfiguration). Power outages are covered by NFR-REL-02. | E | T |
| NFR-REL-02 | After a power outage, the station shall resume operation on its own, with the correct time (RTC) and without losing its configuration. | E | T |
| NFR-REL-03 | A power outage shall not corrupt records already written; at most the record in progress may be lost. | E | T |
| NFR-REL-04 | If the firmware hangs, the station shall restart on its own. | E | T |
| NFR-REL-05 | A network outage of up to 7 days (about 2,000 records at the default interval) shall not cause data loss: everything stays on the SD card and is resent to MQTT. Card capacity is defined in NFR-RES-02 and in the bill of materials. | E | T |
| NFR-REL-06 | A sensor that is disconnected and reconnected shall be read again without restarting the station. | D | T |

### 2.4 Installation environment (NFR-ENV)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| NFR-ENV-01 | The unit shall operate outdoors, exposed to sun, rain and wind, within the temperature range of NFR-ENV-03. The project's reference installation is in Campinas, Brazil (NFR-ACC-03). | E | D |
| NFR-ENV-02 | The electronics shall be housed in an IP65 enclosure. | E | I |
| NFR-ENV-03 | The electronics shall operate within the temperature range expected inside the enclosure, from -5 to 70 °C (operating limits of each component to be confirmed). | E | A |
| NFR-ENV-04 | External cables shall enter the enclosure through cable glands, keeping it sealed. | E | I |
| NFR-ENV-05 | The installation shall consider surge protection and mast grounding (applicable recommendations to be confirmed). | D | I |

### 2.5 Power (NFR-PWR)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| NFR-PWR-01 | The station shall be powered from the mains, through an off-the-shelf power supply. | E | I |
| NFR-PWR-02 | The average power consumption shall be measured and documented (baseline for a future solar-powered extension). | E | T |

### 2.6 Usability (NFR-USE)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| NFR-USE-01 | The web page shall work well on phones and computers (responsive layout). | E | D |
| NFR-USE-02 | The web page shall work without internet access (no files loaded from external servers). | E | T |
| NFR-USE-03 | The web page shall use SI units. | E | I |
| NFR-USE-04 | The web page shall be available in Portuguese and English, with the language selected in the configuration. | E | D |
| NFR-USE-05 | Initial setup shall be possible with a phone only, without rebuilding the firmware. | E | D |

### 2.7 Hardware resources (NFR-RES)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| NFR-RES-01 | The firmware shall fit an ESP32-S3 with 8 MB of flash, with two application partitions (for OTA) and one LittleFS partition. | E | I |
| NFR-RES-02 | The SD card shall hold at least 1 year of records (CSV and JSON Lines; record size to be confirmed). | E | A |

### 2.8 Maintainability and portability (NFR-MNT)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| NFR-MNT-01 | The firmware shall be organized in layers: application, services, sensor libraries and platform port. | E | I |
| NFR-MNT-02 | Each sensor shall have its own library, with a platform-independent C99 core and an ESP-IDF port. | E | I |
| NFR-MNT-03 | The libraries shall be publishable as ESP-IDF components. | E | I |
| NFR-MNT-04 | Code, comments and repository documentation shall be in English. | E | I |
| NFR-MNT-05 | Code shall be simple and readable, since it is quoted in a book and an article series; every quoted excerpt shall match the corresponding release tag. | E | I |

### 2.9 Documentation and reproducibility (NFR-DOC)

| ID | Requirement | Pri. | Ver. |
|---|---|---|---|
| NFR-DOC-01 | The repository shall include a bill of materials with items, quantities and substitutes, without suppliers. | E | I |
| NFR-DOC-02 | Each module shall have a simplified test report. | E | I |
| NFR-DOC-03 | *Removed: covered by constraints CON-03 and CON-07.* | — | — |
| NFR-DOC-04 | Firmware and libraries under the MIT license; hardware under CERN-OHL v2; mechanical parts under CERN-OHL or CC BY-SA 4.0. | E | I |

---

## 3. Constraints (CON)

| ID | Constraint |
|---|---|
| CON-01 | ESP32-S3 microcontroller with at least 8 MB of flash (16 MB recommended). |
| CON-02 | Firmware in C with plain ESP-IDF and FreeRTOS (no Arduino). |
| CON-03 | Ready-made modules only; no custom printed circuit board in the main version. |
| CON-04 | A single outdoor unit, with no local display. |
| CON-05 | Wi-Fi as the only connectivity. |
| CON-06 | Sensors as listed in the bill of materials, with the substitutes provided there. |
| CON-07 | Radiation shield and enclosure either off-the-shelf, homemade or printed by a service (no 3D printer required). |

---

## 4. Out of scope for the main version (OOS)

| ID | Out of scope | Status |
|---|---|---|
| OOS-01 | Local display. | Not planned. |
| OOS-02 | LoRa or any network other than Wi-Fi. | Not planned. |
| OOS-03 | Battery or solar operation. | Planned as a later extension. |
| OOS-04 | Web page authentication, MQTT over TLS and signed OTA. | Planned as a later extension. |
| OOS-05 | Custom printed circuit board. | Planned as a later extension. |
| OOS-06 | 3D-modeled and printed parts. | Planned as a later extension. |
| OOS-07 | Laboratory calibration or certification; the reference is the manufacturer's specification and comparison with official stations. | Not planned. |
| OOS-08 | Unit tests, CI and static analysis as documented deliverables. | Not planned. |
| OOS-09 | More than one unit, or a network of stations. | Not planned. |
| OOS-10 | Weather forecasting. | Not planned. |
