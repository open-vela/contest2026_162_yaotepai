/****************************************************************************
 * chips/sf32lb58/flash_get_freq_stub.c
 *
 * Stub for flash_get_freq() — the real implementation lives in the
 * SiFli BSP which we don't include for nxboot.
 *
 * Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: flash_get_freq
 *
 * Description:
 *   Return the flash clock frequency in Hz for the given clock module
 *   and divider.  The real implementation reads from the clock tree;
 *   this stub returns a fixed 48 MHz which is safe for QSPI NAND
 *   at the divider configured in BSP_Board_PreInit().
 *
 ****************************************************************************/

uint32_t flash_get_freq(int clk_module, uint16_t clk_div, uint8_t hcpu)
{
  /* DLL2 is 288 MHz, divider is typically 6 → 48 MHz */

  (void)clk_module;
  (void)hcpu;

  if (clk_div == 0)
    {
      clk_div = 1;
    }

  return 288000000 / clk_div;
}
