#!/bin/bash
# Auto-detect USB serial port (macOS or Linux)
PORT=$(ls /dev/cu.usbserial* /dev/cu.usbmodem* /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | head -n1)
if [ -z "$PORT" ]; then
    echo "Error: No USB serial device found. Make sure your ESP32 is connected."
    exit 1
fi
echo "Flashing to $PORT..."
arduino-cli upload -p "$PORT" --fqbn esp32:esp32:esp32s3:UploadSpeed=115200 touch-grass.ino
