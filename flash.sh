#!/bin/bash
arduino-cli upload -p /dev/ttyACM0 --fqbn esp32:esp32:esp32s3:UploadSpeed=115200 TouchGrass.ino
