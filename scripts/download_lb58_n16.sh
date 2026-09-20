#!/bin/bash

WORK_PATH="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd)"
CURR_PATH="$(pwd)"
cd "$WORK_PATH"

# ensure pip install -r apps/boot/nxboot/tools/requirements.txt
python ../../apps/boot/nxboot/tools/nximage.py --primary -v ../../cmake_out/sf32lb58-lcd_n16r32n1_qspi_bt_nsh/nuttx.bin nuttx-slot-primary-n16.bin

cp ../../cmake_out/sf32lb58_n16r32n1-nxboot_boot/nuttx.bin nxboot-n16.bin

JLinkExe -CommandFile download_lb58_n16.jlink

cd "$CURR_PATH"
