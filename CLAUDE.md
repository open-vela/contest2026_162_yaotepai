# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is team yaotepai (#162) contest repository for the 2026 openvela AI Hardware Developer Contest. openvela is an embedded RTOS based on Apache NuttX, targeting SiFli SF32LB58 Cortex-M33 microcontrollers. The repo is managed via Google's `repo` tool with a manifest that symlinks our code into the openvela build tree.

## Repository Structure

- `app/` — Native NuttX applications (C, built with Make + Kconfig)
- `quickapp/` — QuickApp UI applications (declarative `.ux` files with template/style/script)
- `board/` — Board Support Package (BSP) for custom hardware
- `chips/` — Chip-level Kconfig definitions (sf32lb58 added by us; not in upstream openvela.xml)
- `logs/` — AI Coding session logs (must be committed; format: `logs/<github-login>/<date>/<tool>__<sid>.jsonl`)
- `contest2026_162_yaotepai.xml` — repo manifest; defines `<linkfile>` mappings from our dirs into the openvela tree
- `openvela.xml` — base manifest included by ours; pulls the full openvela source

## Build System

All builds run from the **openvela workspace root** (parent directory of this repo, where `nuttx/`, `apps/`, `packages/`, `vendor/` live). Our subdirectories are symlinked in by the manifest.

```bash
# Full build (from workspace root)
./build.sh <board-config-path> [-j8]

# Interactive Kconfig menu
./build.sh <board-config-path> menuconfig

# Clean build
./build.sh <board-config-path> distclean
```

Board config paths follow the pattern `vendor/openvela/boards/<board-name>/configs/<config>/defconfig`. Our board is at `vendor/openvela/boards/contest2026_162_board/configs/nsh/defconfig`.

### Build configuration layers

- **Kconfig** — feature selection; app Kconfig files use `CONFIG_LVX_USE_DEMO_CONTEST2026_*` prefixes, board Kconfig uses `ARCH_BOARD_CONTEST2026_*`
- **defconfig** — minimal board config enabling the desired Kconfig symbols
- **Makefile / Make.defs** — NuttX app registration (`CONFIGURED_APPS`, `Application.mk`)
- **CMakeLists.txt** — alternative CMake build (`nuttx_add_application`, `nuttx_add_library`)

## Adding a New Application

1. Create directory under `app/<app_name>/`
2. Add `Kconfig` with config symbol (prefix `LVX_USE_DEMO_CONTEST2026_162_`)
3. Add `Makefile` using `$(APPDIR)/Make.defs` and `$(APPDIR)/Application.mk`
4. Add `Make.defs` to register with `CONFIGURED_APPS`
5. Optionally add `CMakeLists.txt` using `nuttx_add_application()`
6. Add `<linkfile>` in `contest2026_162_yaotepai.xml` mapping to `packages/demos/contest2026_162_<app_name>`
7. Enable the config symbol in the board's defconfig or via `menuconfig`

## Adding a New Board

1. Create directory under `board/<board_name>/`
2. Provide `configs/<config>/defconfig` with at minimum the board's `ARCH_BOARD_*` symbol
3. Add `src/board_boot.c` implementing `openvela_board_initialize()`
4. Add `<linkfile>` in the manifest mapping to `vendor/openvela/boards/contest2026_162_<board_name>`

## Chip Support (chips/sf32lb58)

The `chips/` directory contains Kconfig for SiFli SF32LB58 (Cortex-M33 with FPU, MPU, I/D-cache). This is **not** included in the upstream `openvela.xml` manifest — we added it. To use it, the chip Kconfig must be sourced by the build system (typically via a board's Kconfig `source` directive).

## Key Patterns

- openvela NuttX apps are plain C with `main()` entry; they register via Kconfig + Makefile conventions
- Board init is a single `openvela_board_initialize()` function called early in boot
- The manifest `<linkfile>` mechanism means we never edit files outside our repo — all integration is via symlinks
- QuickApps use a declarative UX framework: `.ux` files with `<template>`, `<style>`, `<script>` sections; `manifest.json` declares routing and metadata

## Submission

- Code deadline: September 20, 2026
- Submit via PR to this repo (self-merge allowed)
- `logs/` directory must contain real AI coding session exports before final submission
- README.md should be rewritten with project-specific documentation before submission

---

## Board & Chip Porting Reference

This section documents the current state of the `chips/` and `board/` directories, known issues, and NuttX porting best practices.

### Current Porting Status

**Chip: `chips/sf32lb58/`** — SiFli SF32LB58 SoC (dual/triple Cortex-M33 ARMv8-M)

The chip directory provides NuttX drivers for the SF32LB58 SoC, adapted from the SiFli vendor SDK (originally RT-Thread based). Key files:

| File | Purpose |
|------|---------|
| `sifli_start.c` | Chip startup, BSS/DATA init, `nx_start` entry |
| `sifli_irq.c` | NVIC IRQ init, enable/disable, priority |
| `sifli_uart.c` | UART driver (NuttX `uart_ops_s`) |
| `sifli_lowput.c` | Low-level putc, early serial init |
| `sifli_oneshot.c` | Tickless timer (LPTIM + SysTick fallback) |
| `sf32lb58_allocateheap.c` | Heap allocation, PSRAM init |
| `sifli_gpio.c` | GPIO driver |
| `sifli_i2c.c` | I2C master driver |
| `sifli_spi.c` | SPI bus driver |
| `sf32lb_adc.c` | ADC driver |
| `sf32lb_flash.c` | NOR flash MTD driver (QSPI) |
| `sf32lb_iwdg.c` | Independent watchdog |
| `sf32lb_pwm.c` | PWM driver |
| `sf32lb_rtc.c` | RTC driver |
| `sf32lb_timer.c` | Hardware timer driver |
| `sf32lb_usbdev.c` | USB device controller (MUSB) |
| `sfconfig.h` | Translates NuttX `CONFIG_*` → vendor `BSP_USING_*` macros |
| `rtconfig.h` | RT-Thread compatibility shim (includes `nuttx/config.h` + `sfconfig.h`) |
| `ipc_queue/` | Inter-processor communication (HCPU↔LCPU↔ACPU via HW mailbox) |

**Board: `board/sf32lb58/sf32lb58-lcd_a128r32n1_qspi/`** — Wearable/smartwatch board

The board provides BSP for a 390×450 LCD smartwatch with PSRAM, NOR flash, touch panel, Bluetooth, USB, and more. Key files:

| File | Purpose |
|------|---------|
| `src/bsp_init.c` | Multi-stage boot: `BSP_Board_PreInit()` (clocks, PSRAM, flash), `BSP_IO_Init()` |
| `src/bsp_pinmux.c` | Pin mux for HCPU and LCPU peripherals |
| `src/bsp_power.c` | Power up/down for LCD, touch, SD, PSRAM |
| `src/sifli_ap.c` | `board_late_initialize()` → `sf32lb58_lcd_bringup()` (all peripheral init) |
| `src/bsp_lcd_tp.c` | LCD and touch panel bringup |
| `src/sf32lb58_buttons.c` | Button driver |
| `drivers/lcd/` | CO5300 LCD + SF32LB LCDC drivers |
| `drivers/input/` | FT6146 touch controller driver |
| `scripts/ld.script` | Memory layout: flash 0x12010000 (16MB), SRAM 0x20000000 (2MB), PSRAM 0x60000000 (8MB) |

**Board: `board/contest_board/`** — Placeholder skeleton (not functional)

### Known Issues & Gaps

**Critical:**

1. **`bsp_psramc_init()` is a stub** — returns 0 without initializing PSRAM controller. `CONFIG_MM_REGIONS=2` expects PSRAM as second heap, but PSRAM is never actually initialized. Will cause runtime failures if PSRAM memory is accessed.

2. **I2C bus mismatch for touch panel** — `bsp_pinmux.c` maps touch to I2C4 (PA59/PA60), but `sifli_ap.c` re-pins to I2C1 (PA30/PA33) at runtime. The pinmux I2C4 assignment is dead code.

3. **`BSP_Power_Up()` / `BSP_IO_Power_Down()` are stubs** — PSRAM enter/exit low-power calls are commented out. Power management is incomplete.

**Non-Standard Patterns:**

4. **RT-Thread artifacts** — `rtconfig.h` includes, `rt_hw_flash_init()`, `RTMSymTab`/`FSymTab` linker sections, `#ifdef BSP_USING_RTTHREAD` guards. Port is not fully cleaned up.

5. **SF32LB52 references in SF32LB58 code** — Header guards, file path comments, and CMake comments still reference SF32LB52 (the chip this was ported from).

6. **Duplicate pinmux entries** — UART1 pins configured twice (once in `#ifdef BSP_ENABLE_MPI4`, once unconditionally). I2C6 pins configured twice.

7. **Editor backup files committed** — `~` files (`CMakeLists.txt~`, `defconfig~`, `Kconfig~`, etc.) should be in `.gitignore`.

8. **`defconfig` uses inline `#` comments** — Kconfig defconfig format does not support inline comments; these may cause parsing issues.

9. **`defconfig` has duplicate entries** — `CONFIG_BUILTIN=y`, `CONFIG_NSH_BUILTIN_APPS=y`, `CONFIG_PIPES=y` appear twice.

10. **Missing `Makefile` sources** — `src/Makefile` only lists `sifli_ap.c` and `sf32lb58_buttons.c`; missing `bsp_init.c`, `bsp_lcd_tp.c`, `bsp_pinmux.c`, `bsp_power.c`, `sifli_gpio.c`. Legacy Make builds will be incomplete.

11. **Kconfig MPI5 duplicate label** — `BSP_MPI5_MODE_3` and `BSP_MPI5_MODE_4` both labeled "OPSRAM"; mode 4 should be "HPSRAM".

12. **SPI3/SPI4 not implemented** — Kconfig defines `BSP_USING_SPI3`/`BSP_USING_SPI4` but driver only handles SPI1/SPI2.

13. **Hardcoded UART1 for early serial** — `sifli_lowput.c` hardcodes `hwp_usart1` and pinmux; should be configurable.

14. **`sf32lb_lcd_getrun()` has `DEBUGASSERT(0)`** — read-pixel path will crash if called.

### NuttX Board Porting Best Practices

**Standard directory structure:**

```
boards/<arch>/<chip>/<board>/
  configs/<config>/defconfig    # Minimal config fragment
  src/
    Kconfig                     # Board-specific options
    Makefile                    # Board source list
    Make.defs                   # Board CFLAGS
    board.h                     # Pin definitions, macros
    board_initialize.c          # openvela_board_initialize()
    board_bringup.c             # board_late_initialize()
  scripts/
    ld.script                   # Linker script
    Make.defs                   # Toolchain flags
  CMakeLists.txt                # CMake support
```

**Boot sequence:**

1. Reset vector → chip startup (`sifli_start.c`)
2. `arm_boot()` — FPU enable, cache enable
3. `BSP_Board_PreInit()` → `openvela_board_initialize()` — clocks, PSRAM, flash, pinmux, power
4. `nx_start()` — OS kernel init (scheduler, idle task, memory)
5. `board_late_initialize()` — filesystems, drivers, apps

**Key rules for chip-level code:**

- Own hardware register definitions and register-level drivers
- Handle DMA setup, interrupt routing, clock gating
- Expose API like `up_uart_initialize()` or `sf32lb58_gpioconfig()`
- Use `sfconfig.h` to bridge NuttX CONFIG_* to vendor BSP_USING_* macros

**Key rules for board-level code:**

- Call chip-level APIs with board-specific parameters (pin numbers, bus speeds)
- Configure all pins in `BSP_PIN_Init()` before using peripherals
- Initialize clocks once in `BSP_Board_PreInit()` — ensure UART baud rate generator matches APB clock
- Mount filesystems and register drivers in `board_late_initialize()`
- Document pin usage in comments or a pin table

**Common pitfalls to avoid:**

- **FPU context save/restore** — EXC_RETURN must handle lazy stacking on Cortex-M with FPU
- **DMA buffer alignment** — must be cache-line aligned (32 bytes on Cortex-M33 with D-cache)
- **Console UART must be ready before `nx_start()`** — or you get silent boot
- **Clock mismatch** — if Kconfig assumes one HCLK but board sets another, baud rates and timers break
- **Stack sizes** — check `CONFIG_IDLETHREAD_STACKSIZE` and `CONFIG_PTHREAD_STACK_DEFAULT`
- **MPU regions** — must cover all memory accessed by the OS or you get MemManage faults
- **Kconfig dependency loops** — board depends on chip, chip depends on arch; no cycles

### openvela vs Upstream NuttX Differences

| Aspect | openvela | Upstream NuttX |
|--------|----------|----------------|
| Board path | `vendor/openvela/boards/<board>/` | `boards/<arch>/<chip>/<board>/` |
| Init function | `openvela_board_initialize()` | `board_initialize()` |
| App location | `packages/demos/` via manifest `<linkfile>` | `apps/` directly |
| Kconfig prefix | `CONFIG_LVX_USE_DEMO_CONTEST2026_*` | `CONFIG_<APPNAME>` |
| Build entry | `build.sh <board-config-path>` | `tools/configure.sh` + `make` |
| Repo management | Google `repo` tool with XML manifests | Single git repo |
| UI framework | QuickApp `.ux` files | Not applicable |
