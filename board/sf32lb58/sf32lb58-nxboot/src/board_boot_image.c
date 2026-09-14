/****************************************************************************
 * board/sf32lb58/sf32lb58-nxboot/src/board_boot_image.c
 *
 * Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
 * SPDX-License-Identifier: Apache-2.0
 *
 * board_boot_image() for SF32LB58 nxboot: loads firmware from NAND to PSRAM
 * and jumps to it.  PSRAM must NOT be re-initialized here — the application
 * code and data have already been copied from NAND to PSRAM by the time
 * this function runs.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <debug.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <sys/boardctl.h>
#include <nuttx/irq.h>
#include <nuttx/cache.h>
#include <arch/barriers.h>

#include "nvic.h"
#include "arm_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Application load address in PSRAM.
 * Must match the application's linker script origin.
 * PSRAM starts at 0x60000000; first 4MB reserved for SDK regions;
 * application code goes at 0x60400000.
 */

#define SF32LB58_APP_PSRAM_LOAD_ADDR  0x60400000

/* Maximum image size we can load (12 MB, leaving room for heap) */

#define SF32LB58_APP_MAX_IMAGE_SIZE   (12U * 1024U * 1024U)

/* Read buffer size for NAND→PSRAM copy */

#define SF32LB58_COPY_BUF_SIZE        4096

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* ARM vector table: first two words of a Cortex-M image */

struct arm_vector_table
{
  uint32_t spr;   /* Stack pointer on reset */
  uint32_t reset; /* Pointer to reset exception handler */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cleanup_arm_nvic
 *
 * Description:
 *   Acknowledge and disable all interrupts in NVIC.
 *
 ****************************************************************************/

static void cleanup_arm_nvic(void)
{
  int i;

  UP_ISB();
  up_irq_disable();

  for (i = 0; i < NR_IRQS; i += 32)
    {
      putreg32(0xffffffff, NVIC_IRQ_CLEAR(i));
    }

  for (i = 0; i < NR_IRQS; i += 32)
    {
      putreg32(0xffffffff, NVIC_IRQ_CLRPEND(i));
    }
}

/****************************************************************************
 * Name: systick_disable
 *
 * Description:
 *   Disable the SysTick system timer.
 *
 ****************************************************************************/

static void systick_disable(void)
{
  putreg32(0, NVIC_SYSTICK_CTRL);
  putreg32(NVIC_SYSTICK_RELOAD_MASK, NVIC_SYSTICK_RELOAD);
  putreg32(0, NVIC_SYSTICK_CURRENT);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_boot_image
 *
 * Description:
 *   Called by nxboot (via BOARDIOC_BOOT_IMAGE) to load the application
 *   image from NAND flash into PSRAM and jump to it.
 *
 *   IMPORTANT: PSRAM is NOT re-initialized here.  The PSRAM controller
 *   was set up during early boot (HAL_PreInit → bsp_psramc_init).
 *   Re-initializing PSRAM would destroy the application code and data
 *   that we copy from NAND.
 *
 * Input Parameters:
 *   path     - Path to the NAND MTD partition (e.g. "/dev/ota0")
 *   hdr_size - Size of the nxboot image header (CONFIG_NXBOOT_HEADER_SIZE)
 *
 ****************************************************************************/

int board_boot_image(const char *path, uint32_t hdr_size)
{
  static struct arm_vector_table vt;
  struct file file;
  ssize_t bytes;
  uint32_t image_size;
  uint32_t remaining;
  uint32_t chunk;
  uint32_t dest;
  off_t src_off;
  int ret;
  uint8_t buf[SF32LB58_COPY_BUF_SIZE];

  syslog(LOG_INFO, "board_boot_image: loading %s (hdr_size=0x%lx)\n",
         path, (unsigned long)hdr_size);

  ret = file_open(&file, path, O_RDONLY | O_CLOEXEC);
  if (ret < 0)
    {
      syslog(LOG_ERR, "Failed to open %s: %d\n", path, ret);
      return ret;
    }

  /* Read the image size from the header (offset 4, uint32_t) */

  bytes = file_pread(&file, &image_size, sizeof(image_size), 4);
  if (bytes != sizeof(image_size))
    {
      syslog(LOG_ERR, "Failed to read image size: %ld\n", (long)bytes);
      file_close(&file);
      return bytes < 0 ? (int)bytes : -1;
    }

  if (image_size == 0 || image_size > SF32LB58_APP_MAX_IMAGE_SIZE)
    {
      syslog(LOG_ERR, "Invalid image size: 0x%08lx\n",
             (unsigned long)image_size);
      file_close(&file);
      return -EINVAL;
    }

  syslog(LOG_INFO, "board_boot_image: image_size=0x%08lx, copying to "
         "PSRAM @ 0x%08lx\n",
         (unsigned long)image_size,
         (unsigned long)SF32LB58_APP_PSRAM_LOAD_ADDR);

  /* Read the ARM vector table (SP + reset handler) from after the header */

  bytes = file_pread(&file, &vt, sizeof(vt), hdr_size);
  if (bytes != sizeof(vt))
    {
      syslog(LOG_ERR, "Failed to read vector table: %ld\n", (long)bytes);
      file_close(&file);
      return bytes < 0 ? (int)bytes : -1;
    }

  syslog(LOG_INFO, "board_boot_image: VT sp=0x%08lx reset=0x%08lx\n",
         (unsigned long)vt.spr, (unsigned long)vt.reset);

  /* Copy the entire image from NAND to PSRAM.
   * We read from the NAND partition starting at hdr_size (after header)
   * and write to PSRAM at the load address.
   *
   * NOTE: We cannot use file_pread directly into PSRAM because the
   * NAND MTD driver uses its own page buffers.  We read into a SRAM
   * buffer and memcpy to PSRAM.
   */

  src_off = (off_t)hdr_size;
  dest = SF32LB58_APP_PSRAM_LOAD_ADDR;
  remaining = image_size;

  while (remaining > 0)
    {
      chunk = remaining > SF32LB58_COPY_BUF_SIZE ?
              SF32LB58_COPY_BUF_SIZE : remaining;

      bytes = file_pread(&file, buf, chunk, src_off);
      if (bytes != (ssize_t)chunk)
        {
          syslog(LOG_ERR,
                 "Failed to read image at offset 0x%08lx: %ld\n",
                 (unsigned long)src_off, (long)bytes);
          file_close(&file);
          return bytes < 0 ? (int)bytes : -1;
        }

      memcpy((void *)(uintptr_t)dest, buf, chunk);

      src_off += chunk;
      dest += chunk;
      remaining -= chunk;
    }

  file_close(&file);

  syslog(LOG_INFO,
         "board_boot_image: copy complete, jumping to PSRAM...\n");

  /* *** DO NOT re-initialize PSRAM here ***
   * The application's .text, .data, and .bss sections are now in PSRAM.
   * Any PSRAM controller re-initialization would destroy them.
   */

  systick_disable();
  cleanup_arm_nvic();

#ifdef CONFIG_ARMV8M_DCACHE
  up_disable_dcache();
#endif
#ifdef CONFIG_ARMV8M_ICACHE
  up_disable_icache();
#endif

#ifdef CONFIG_ARM_MPU
  mpu_control(false, false, false);
#endif

  /* Set main and process stack pointers, jump to reset handler.
   * Use the vector table we read from the image (which points to
   * addresses relative to the PSRAM load address).
   */

  __asm__ __volatile__("\tmsr msp, %0\n"
                       "\tmsr control, %1\n"
                       "\tisb\n"
                       "\tmov pc, %2\n"
                       :
                       : "r" (vt.spr), "r" (0), "r" (vt.reset));

  /* Should never reach here */

  return 0;
}
