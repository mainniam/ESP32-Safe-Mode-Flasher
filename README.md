# ESP32 Safe Mode Flasher

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![ESP32-S3](https://img.shields.io/badge/ESP32--S3-Compatible-blue)](https://www.espressif.com/en/products/socs/esp32-s3)

Safety utility that puts ESP32-S3 GPIO pins into high-impedance input state before firmware updates, preventing damage to connected peripherals.

## Why Use This?
GPIO pins retain state during programming, which can:
- Cause WS2812B LEDs to display random colors
- Trigger motors unexpectedly
- Damage sensitive sensors
- Create short circuits

This utility provides a "clean slate" before uploading new firmware.

## Quick Start

### 1. Install
```bash
# Arduino: Add ESP32-S3 board support
# PlatformIO: Create project with esp32-s3-devkitc-1
```

### 2. Upload Safe Mode
```cpp
// Upload ESP32S3_SafeMode.ino
// All pins automatically secured
```

### 3. Monitor Output (115200 baud)
```
ESP32-S3 Safe Mode Active
34 pins secured | 8 pins skipped | All peripherals disabled
System SAFE for programming
```

### 4. Upload Your App & Reset
```bash
# Upload your actual firmware
# Press RESET to exit safe mode
```

## Features
- ✅ Secures all 49 GPIO pins (0-48)
- ✅ Skips system-critical pins (Flash/PSRAM/USB)
- ✅ Disables PWM, RMT, I2C, SPI
- ✅ Low power consumption
- ✅ Detailed serial feedback

## Supported Boards
- ESP32-S3-DEV-KIT-NXRX
- Most ESP32S3 development boards
- Other ESP32 development boards

## License
MIT License. See [LICENSE](LICENSE) for details.

## Contributing
Issues and PRs welcome. See [CONTRIBUTING](CONTRIBUTING.md).

---

**Protect your hardware. Flash safely.**

*Built for the ESP32 community*
