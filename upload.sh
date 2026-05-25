#!/usr/bin/env bash
# Upload depart.ino to ESP32-S3.
# Before flashing, reads current firmware from device and saves it with the
# sketch source to backups/YYYYDDMMHHmmSS.bak/
# Use --no-backup to skip the backup step.

set -euo pipefail

BACKUP=true
for arg in "$@"; do
    [ "$arg" = "--no-backup" ] && BACKUP=false
done

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SKETCH_DIR="$SCRIPT_DIR/depart"
BACKUP_ROOT="$SCRIPT_DIR/backups"
FQBN="esp32:esp32:esp32s3:PSRAM=opi"
FLASH_SIZE="0x400000"  # 4 MB — standard ESP32-S3 flash

# ── Find device port ──────────────────────────────────────────────────────────
PORT=""
for p in /dev/ttyACM* /dev/ttyUSB*; do
    [ -e "$p" ] && { PORT="$p"; break; }
done
if [ -z "$PORT" ]; then
    echo "ERROR: no serial device found — tried /dev/ttyACM* and /dev/ttyUSB*"
    echo "       Make sure the board is plugged in and you're in the 'dialout' group."
    echo "       (sudo usermod -aG dialout \$USER  then re-login)"
    exit 1
fi
echo "Device : $PORT"

# ── Helper ────────────────────────────────────────────────────────────────────
prompt_download_mode() {
    echo ""
    echo "  ╔═══════════════════════════════════════════╗"
    echo "  ║  Put the board in download mode:          ║"
    echo "  ║  1. Hold   BOOT                           ║"
    echo "  ║  2. Press  RESET  (then release it)       ║"
    echo "  ║  3. Release BOOT                          ║"
    echo "  ╚═══════════════════════════════════════════╝"
    read -rp "  Press Enter when ready... "
    echo ""
}

# ── Backup ────────────────────────────────────────────────────────────────────
if $BACKUP; then
    STAMP=$(date +%Y%d%m%H%M%S)
    BACKUP_DIR="$BACKUP_ROOT/${STAMP}.bak"
    mkdir -p "$BACKUP_DIR"
    echo "Backup : $BACKUP_DIR"

    prompt_download_mode
    echo "[1/4] Reading firmware from device (this takes ~30 s)..."
    esptool.py --port "$PORT" --baud 921600 --chip esp32s3 \
        read_flash 0 $FLASH_SIZE "$BACKUP_DIR/firmware.bin"

    echo "[2/4] Saving sketch source..."
    cp -r "$SKETCH_DIR" "$BACKUP_DIR/sketch"
else
    echo "Backup : skipped (--no-backup)"
fi

# ── Compile ───────────────────────────────────────────────────────────────────
COMPILE_STEP=$( $BACKUP && echo "[3/4]" || echo "[1/2]" )
echo "$COMPILE_STEP Compiling..."
arduino-cli compile --fqbn "$FQBN" "$SKETCH_DIR"

# ── Upload ────────────────────────────────────────────────────────────────────
UPLOAD_STEP=$( $BACKUP && echo "[4/4]" || echo "[2/2]" )
prompt_download_mode
echo "$UPLOAD_STEP Uploading..."
arduino-cli upload --fqbn "$FQBN" -p "$PORT" "$SKETCH_DIR"

echo ""
if $BACKUP; then
    echo "Done. Firmware backed up to: $BACKUP_DIR/firmware.bin"
    echo "To restore the backup:"
    echo "  esptool.py --port $PORT --baud 921600 --chip esp32s3 \\"
    echo "      write_flash 0 $BACKUP_DIR/firmware.bin"
else
    echo "Done."
fi
