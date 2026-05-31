#!/usr/bin/env bash
# Compile and upload shake.ino, then stream serial output to a timestamped log file.
# Press Ctrl+C to stop the monitor.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SKETCH_DIR="$SCRIPT_DIR/shake"
LOG_DIR="$SCRIPT_DIR/shake_logs"
FQBN="esp32:esp32:esp32s3:PSRAM=opi,CDCOnBoot=cdc"
LOGFILE=""

on_exit() {
    [ -n "$LOGFILE" ] && echo "" && echo "Log saved to: $LOGFILE"
}
trap on_exit EXIT

# ── Find device port ──────────────────────────────────────────────────────────

PORT=""
for p in /dev/ttyACM* /dev/ttyUSB*; do
    [ -e "$p" ] && { PORT="$p"; break; }
done
if [ -z "$PORT" ]; then
    echo "ERROR: no serial device found — tried /dev/ttyACM* and /dev/ttyUSB*"
    echo "       Make sure the board is plugged in and you're in the 'dialout' group."
    exit 1
fi
echo "Device : $PORT"

# ── Compile & upload ──────────────────────────────────────────────────────────

echo "[1/2] Compiling..."
arduino-cli compile --fqbn "$FQBN" "$SKETCH_DIR"

echo "[2/2] Uploading..."
arduino-cli upload --fqbn "$FQBN" -p "$PORT" "$SKETCH_DIR"

# Wait for the device to reboot and the USB CDC port to re-enumerate.
sleep 2

# ── Monitor → log file ────────────────────────────────────────────────────────

mkdir -p "$LOG_DIR"
LOGFILE="$LOG_DIR/$(date +%Y%m%d_%H%M%S).log"

echo ""
echo "Logging to : $LOGFILE"
echo "Press Ctrl+C to stop."
echo ""

arduino-cli monitor -p "$PORT" -c baudrate=115200 | tee "$LOGFILE" || true
