# LoRaNetLibrary

Unified PlatformIO & Arduino library for LoRa radio communication (`SX126x`/`Ra-01S`) and binary frame protocol serialization (`LoraMsg`) in the LoRaFarmNet ecosystem.

## Features
- **SX126x Driver**: Hardware interface for SX1261/SX1262/Ra-01S transceivers.
- **LoraMsg Protocol**: Compact binary frame serialization, encryption, decryption, and CRC handling.
- **Multi-Platform**: Supports ESP32 (`espressif32`) and ATmega644PA (`atmelavr` / MightyCore).

## Installation
Add to `platformio.ini`:
```ini
lib_deps =
    https://github.com/toogooda/LoRaNetLibrary.git
```
