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
