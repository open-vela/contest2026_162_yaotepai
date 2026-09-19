/****************************************************************************
 * board/sf32lb58/sf32lb58-nxboot/src/sf32lb58_lcd_a128r32n1_qspi.h
 *
 * Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal board header for the nxboot bootloader.
 * Shares BSP code with the main board (bsp_init.c, bsp_pinmux.c, etc.)
 * via the chip-level HAL.
 ****************************************************************************/

#ifndef __BOARD_SF32LB58_SF32LB58_NXBOOT_H
#define __BOARD_SF32LB58_SF32LB58_NXBOOT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int sf32lb58_boot_bringup(void);

#endif /* __BOARD_SF32LB58_SF32LB58_NXBOOT_H */
