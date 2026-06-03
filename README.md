# Makerfabs ESP32-S3 Parallel TFT 3.16" ST7701S — Railway Departure Board

Live UK railway departure board running on the [Makerfabs ESP32-S3 Parallel TFT 3.16"](https://github.com/Makerfabs/ESP32-S3_Parallel_TFT_3.16_ST7701S) board. Fetches departures from [national-rail-api.davwheat.dev](https://national-rail-api.davwheat.dev) and refreshes every minute. Shake the board to enter configuration mode.

## Hardware

| Feature | Details |
|---|---|
| MCU | ESP32-S3, 16 MB Flash, 8 MB OPI PSRAM |
| Display | 320×820 ST7701S, RGB parallel + SPI init |
| IMU | QMI8658 (shake detection) |

### Pin assignments (from schematic v1.0)

**Display**

| Signal | GPIO |
|---|---|
| Backlight (active LOW) | 46 |
| SPI CS / SCK / MOSI | 45 / 39 / 40 |
| DE / VSYNC / HSYNC / PCLK | 7 / 4 / 5 / 6 |
| R0–R4 | 12, 11, 8, 16, 15 |
| G0–G5 | 0, 14, 10, 9, 3, 13 |
| B0–B4 | 48, 47, 1, 21, 41 |

**IMU (QMI8658)**

| Signal | GPIO |
|---|---|
| SDA / SCL | 17 / 18 |

## Environment setup

### 1. arduino-cli

```bash
mkdir -p ~/bin
curl -fsSL "https://downloads.arduino.cc/arduino-cli/nightly/arduino-cli_nightly-latest_Linux_64bit.tar.gz" | tar -xz -C ~/bin/
export PATH="$HOME/bin:$PATH"   # add to ~/.zshrc or ~/.bashrc
```

### 2. ESP32 Arduino core (v3.2.0)

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.2.0
```

> Core 3.x uses ESP-IDF 5.x, which adds `bounce_buffer_size_px` for RGB panels — the fix for PSRAM framebuffer jitter. Do not use core 2.x; it lacks this feature.

### 3. Libraries

```bash
arduino-cli lib install "GFX Library for Arduino"@1.6.5
arduino-cli lib install "ArduinoJson"@7.3.1
```

> GFX 1.6+ renamed color constants to `RGB565_BLACK` etc.; the sketches add short aliases.
> The library's built-in `st7701_type9_init_operations` targets a different panel; the sketches embed the correct Makerfabs 3.16" init sequence directly.

### 4. esptool (for firmware backups)

`upload.sh` reads the current firmware before flashing. esptool is needed for that step (skip with `--no-backup` if you don't need it).

```bash
pip3 install esptool --break-system-packages
```

## Uploading

### Departure board

```bash
./upload.sh           # backup current firmware, compile, flash
./upload.sh --no-backup   # skip backup
./upload.sh --debug       # open serial monitor after flashing
```

### Shake data logger

`shake/` is an auxiliary sketch for recording accelerometer data used to tune the shake-detection thresholds in `depart/`.

```bash
./upload_shake.sh     # compile, flash, stream serial output to shake_logs/
```

## First-run / WiFi configuration

On first boot (or if WiFi credentials are missing), the board starts an access point:

- **SSID:** `DepartBoard`  **Password:** `t123`

Connect and open `http://192.168.4.1` to set your WiFi credentials and station code (3-letter CRS code, e.g. `MZH`). After saving, the board reboots and connects automatically.

To reconfigure later, shake the board while it is showing the departure board. This opens the config portal for 5 minutes, after which it returns to normal operation. Shake again to exit early.

Station CRS codes: [nationalrail.co.uk](https://www.nationalrail.co.uk/stations_destinations/48541.aspx)

## Notes

- **Backlight is active-LOW** — the NPN transistor circuit on GPIO 46 inverts the signal.
- **PSRAM=opi** — OPI PSRAM must be selected; the RGB framebuffer lives in PSRAM.
- **Double buffering** — drawing goes to an off-screen canvas in PSRAM; `screen_flush()` copies it atomically to avoid tearing.
- **Time zone** — NTP is configured for UK time (GMT/BST). Change the `TZ` string in `sync_time()` for other zones.
