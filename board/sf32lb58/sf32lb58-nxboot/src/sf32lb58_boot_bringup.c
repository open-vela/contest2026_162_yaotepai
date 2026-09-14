/****************************************************************************
 * board/sf32lb58/sf32lb58-nxboot/src/sf32lb58_boot_bringup.c
 *
 * Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bootloader board bringup for SF32LB58 nxboot.
 * Minimal initialization: UART console + NAND MTD partitions only.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <syslog.h>
#include <errno.h>
#include <sys/stat.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>

#ifdef CONFIG_MTD
#  include "sf32lb_nand.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* NAND partition layout for nxboot.
 * Adjust offsets/sizes to match your flash layout.
 * These are byte offsets within the NAND flash.
 */

#ifdef CONFIG_MTD
#  define SF32LB58_NAND_PART0_OFFSET  0x68000000  /* Primary slot */
#  define SF32LB58_NAND_PART0_SIZE    0x00800000  /* 8 MB */
#  define SF32LB58_NAND_PART1_OFFSET  0x68800000  /* Secondary slot */
#  define SF32LB58_NAND_PART1_SIZE    0x00800000  /* 8 MB */
#  define SF32LB58_NAND_PART2_OFFSET  0x69000000  /* Tertiary slot */
#  define SF32LB58_NAND_PART2_SIZE    0x00800000  /* 8 MB */
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sf32lb58_boot_bringup
 *
 * Description:
 *   Minimal bringup for the nxboot bootloader.
 *   Initializes NAND flash and registers partitions for OTA.
 *
 ****************************************************************************/

int sf32lb58_boot_bringup(void)
{
  int ret = OK;

#ifdef CONFIG_MTD
  {
    static const char *const partnames[] =
    {
      CONFIG_NXBOOT_PRIMARY_SLOT_PATH,
      CONFIG_NXBOOT_SECONDARY_SLOT_PATH,
      CONFIG_NXBOOT_TERTIARY_SLOT_PATH
    };

    static const uint32_t part_offsets[] =
    {
      SF32LB58_NAND_PART0_OFFSET,
      SF32LB58_NAND_PART1_OFFSET,
      SF32LB58_NAND_PART2_OFFSET
    };

    static const uint32_t part_sizes[] =
    {
      SF32LB58_NAND_PART0_SIZE,
      SF32LB58_NAND_PART1_SIZE,
      SF32LB58_NAND_PART2_SIZE
    };

    ret = sf32lb_nand_register_partitions(
            partnames, part_offsets, part_sizes, 3);
    if (ret < 0)
      {
        syslog(LOG_ERR, "ERROR: NAND partition registration failed: %d\n",
               ret);
      }
  }
#endif

  return ret;
}

/****************************************************************************
 * Name: board_early_initialize
 ****************************************************************************/

#ifdef CONFIG_BOARD_EARLY_INITIALIZE
void board_early_initialize(void)
{
  /* Nothing extra needed — HAL_PreInit() handles clocks, PSRAM, pinmux */
}
#endif

/****************************************************************************
 * Name: board_late_initialize
 ****************************************************************************/

#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void)
{
  sf32lb58_boot_bringup();
}
#endif

/****************************************************************************
 * Name: board_app_initialize
 ****************************************************************************/

int board_app_initialize(uintptr_t arg)
{
#ifdef CONFIG_BOARD_LATE_INITIALIZE
  return OK;
#else
  return sf32lb58_boot_bringup();
#endif
}

/****************************************************************************
 * Name: board_app_finalinitialize
 ****************************************************************************/

#ifdef CONFIG_BOARDCTL_FINALINIT
int board_app_finalinitialize(uintptr_t arg)
{
  return 0;
}
#endif

/****************************************************************************
 * Name: board_reset
 ****************************************************************************/

#ifdef CONFIG_BOARDCTL_RESET
int board_reset(int status)
{
  (void)status;
  up_systemreset();
  return OK;
}
#endif
