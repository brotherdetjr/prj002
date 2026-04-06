# Makerfabs ESP32-S3 Parallel TFT 3.16" ST7701S — Demo

Rainbow gradient + beep demo for the [Makerfabs ESP32-S3 Parallel TFT 3.16"](https://github.com/Makerfabs/ESP32-S3_Parallel_TFT_3.16_ST7701S) board.

## Hardware

| Feature | Details |
|---|---|
| MCU | ESP32-S3, 16 MB Flash, 8 MB OPI PSRAM |
| Display | 320×820 ST7701S, RGB parallel + SPI init |
| Audio | MAX98357A I2S amplifier (3.2 W) |
| IMU | QMI8658 (not used in this demo) |

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

**Audio (MAX98357A)**

| Signal | GPIO |
|---|---|
| BCLK | 44 |
| LRCLK | 43 |
| DOUT (ESP32 → amp) | 19 |

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

### 3. GFX Library for Arduino (v1.6.5)

```bash
arduino-cli lib install "GFX Library for Arduino"@1.6.5
```

> **Important:** The library's built-in `st7701_type9_init_operations` targets a different panel.
> The demo sketch embeds the correct Makerfabs 3.16" init sequence directly, so no library patching is needed.
> GFX 1.6.5 renamed color constants to `RGB565_BLACK` etc.; the sketch adds short aliases.

### 4. esptool (for flashing)

```bash
pip3 install esptool --break-system-packages
```

### 5. Flash helper script

The ESP32-S3 USB-CDC interface does not support RTS/DTR auto-reset after firmware starts running. This script works around that:

```bash
cat > ~/bin/flash_no_rts.py << 'EOF'
"""Flash ESP32-S3 without RTS/DTR — USB-CDC/JTAG workaround."""
import sys, os
BUNDLED = os.path.expanduser("~/.arduino15/packages/esp32/tools/esptool_py/4.9.dev3")
sys.path.insert(0, BUNDLED)
for key in list(sys.modules):
    if key == "esptool" or key.startswith("esptool."):
        del sys.modules[key]
import serial.serialposix as _sp
_orig_rts = _sp.Serial._update_rts_state
_orig_dtr = _sp.Serial._update_dtr_state
def _safe_rts(self):
    try: _orig_rts(self)
    except OSError as e:
        if e.errno != 71: raise
def _safe_dtr(self):
    try: _orig_dtr(self)
    except OSError as e:
        if e.errno != 71: raise
_sp.Serial._update_rts_state = _safe_rts
_sp.Serial._update_dtr_state = _safe_dtr
import esptool
esptool._main()
EOF
chmod +x ~/bin/flash_no_rts.py
```

## Build

```bash
export PATH="$HOME/bin:$PATH"

arduino-cli compile --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc" --output-dir /tmp/demo_build ~/prj002/demo/
```

## Flash

Flashing requires putting the board into bootloader mode manually each time (auto-reset does not work over USB-CDC after firmware starts):

1. Hold the **BOOT** button
2. Press and release **RESET**
3. Release **BOOT**
4. Immediately run:

```bash
python3 ~/bin/flash_no_rts.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 --before no-reset --after hard-reset write-flash -z 0x0 /tmp/demo_build/demo.ino.bootloader.bin 0x8000 /tmp/demo_build/demo.ino.partitions.bin 0xe000 ~/.arduino15/packages/esp32/hardware/esp32/3.2.0/tools/partitions/boot_app0.bin 0x10000 /tmp/demo_build/demo.ino.bin
```

> The board re-enumerates after the upload completes and boots the new firmware automatically.

## Notes

- **Backlight is active-LOW** — the NPN transistor circuit on GPIO 46 inverts the signal.
- **Init sequence** — the library's `st7701_type9` was written for a different manufacturer's panel. `demo.ino` defines `mf_st7701_init[]` with the correct sequence for this 3.16" Makerfabs panel, taken from the [official Makerfabs patched library](https://github.com/Makerfabs/ESP32-S3_Parallel_TFT_3.16_ST7701S/tree/main/examples/arduino/lib/GFX_Library_for_Arduino).
- **PSRAM=opi** — OPI PSRAM must be selected; the RGB framebuffer lives in PSRAM.
