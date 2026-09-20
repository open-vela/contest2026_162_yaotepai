---
name: sf32lb58-flash-debug
description: "Flash, debug, and monitor SiFli SF32LB58 boards via J-Link and serial ports. Use when the user wants to: (1) flash nuttx.bin to the board, (2) start a GDB debug session, (3) connect to board serial output (HCPU/LCPU), or (4) troubleshoot board boot issues."
---

# SF32LB58 Flash & Debug

## Workspace Layout

- **Workspace root**: `/home/hongbo/Developer/Embedded/OpenVela`
- **Contest repo**: `contest2026_162_yaotepai/`
- **JLink script**: `download_lb58_nand.jlink` (at workspace root)
- **Build output (CMake)**: `cmake_out/<build-dir>/nuttx.bin`
- **Build output (Make)**: `nuttx/nuttx.bin`

## Flash

The board uses J-Link with NAND flash at address `0x68000000`.

### Step 1: Find the binary

```bash
# CMake build
find cmake_out -name "nuttx.bin" -newer nuttx/CMakeLists.txt | head -1

# Make build
ls -la nuttx/nuttx.bin
```

### Step 2: Copy binary to workspace root

```bash
cp cmake_out/<build-dir>/nuttx.bin ./nuttx.bin
```

### Step 3: Flash

```bash
JLinkExe -CommandFile download_lb58_nand.jlink
```

Or use the convenience script:
```bash
./download_lb58_nand.sh
```

The JLink script (`download_lb58_nand.jlink`):
```
device SF32LB58X_NAND
si SWD
speed 2000
r
loadbin nuttx.bin 0x68000000
exit
```

If flashing fails with timeout errors, try lower speed: change `speed 2000` to `speed 1000` in the jlink file.

## Debug (GDB)

### Start GDB server

```bash
JLinkGDBServer -device SF32LB58X_NAND -if swd -speed 4000
```

If connection is unreliable, reduce speed:
```bash
JLinkGDBServer -device SF32LB58X_NAND -if swd -speed 1000
```

### Connect GDB

In another terminal:
```bash
arm-none-eabi-gdb cmake_out/<build-dir>/nuttx
(gdb) target remote localhost:2331
(gdb) monitor reset
(gdb) load
(gdb) break board_late_initialize
(gdb) continue
```

## Serial Console

The board exposes two USB serial ports:

| Port | CPU | Typical device | Baud |
|------|-----|---------------|------|
| First | LCPU (BT controller) | `/dev/ttyACM0` | 1000000 |
| Second | HCPU (NuttX) | `/dev/ttyACM1` | 1000000 |

A third port `/dev/ttyACM2` may exist — it is for FPGA and uses 115200 baud. Ignore it.

### Identify correct ports

```bash
# List available serial devices
ls /dev/ttyACM*
# Check which one outputs NuttX boot messages
dmesg | grep ttyACM
```

### Connect to HCPU (main console)

Interactive:
```bash
picocom --imap lfcrlf --omap crcrlf -b 1000000 /dev/ttyACM1
```

Log to file (useful for capturing boot output):
```bash
picocom --imap lfcrlf --omap crcrlf -b 1000000 /dev/ttyACM1 -g /tmp/serial_hcpu.log
```

### Connect to LCPU (BT debug)

```bash
picocom --imap lfcrlf --omap crcrlf -b 1000000 /dev/ttyACM0
```

### Exit picocom

Press `Ctrl+A` then `Ctrl+X`.

### Capture boot output (serial + JLink reset)

picocom needs a TTY. When running from scripts or non-interactive shells, wrap with `script`:

```bash
# Start picocom in background (wrapped with script for TTY)
script -q -c "picocom --imap lfcrlf --omap crcrlf -b 1000000 /dev/ttyACM1 -g /tmp/serial_hcpu.log" /dev/null &
PICOCOM_PID=$!

# Reset board via JLink (in separate process)
JLinkExe -CommandFile download_lb58_nand.jlink

# Wait for boot output (6-10 seconds)
sleep 10

# Kill picocom and check log
kill $PICOCOM_PID
cat /tmp/serial_hcpu.log
```

**Important**: Start picocom BEFORE the JLink reset. The reset triggers boot output immediately.

## Troubleshooting

- **No serial output**: Check baud rate (1000000, not 115200). Ensure correct `/dev/ttyACMx` port. Start picocom BEFORE resetting the board.
- **Flash timeout**: Reduce JLink speed to 1000.
- **"Failed to power up DAP"**: JLink can't connect to the CPU. Power cycle the board (unplug USB, wait 5s, reconnect). Then retry with `speed 1000`.
- **GDB can't connect**: Ensure JLinkGDBServer is running. Check USB connection.
- **Board not booting**: Verify `nuttx.bin` is from a successful build. Check HCPU serial for error messages.
