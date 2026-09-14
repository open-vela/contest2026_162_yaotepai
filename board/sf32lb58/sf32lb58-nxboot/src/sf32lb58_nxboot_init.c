/****************************************************************************
 * vendor/sifli/boards/sf32lb58/sf32lb58-nxboot/src/sf32lb58_nxboot_init.c
 *
 * Minimal BSP init for nxboot bootloader.
 * Uses NuttX/HAL headers already available without SiFli SDK includes.
 * Sets up clocks and provides BSP stubs needed by the chip HAL.
 *
 * Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>
#include "bf0_hal.h"

/****************************************************************************
 * Flash divider stubs needed by SiFli HAL drivers
 ****************************************************************************/

static uint16_t mpi1_div = 1;
static uint16_t mpi2_div = 1;
static uint16_t mpi3_div = 3;
static uint16_t mpi4_div = 3;
static uint16_t mpi5_div = 3;

uint16_t BSP_GetFlash1DIV(void) { return mpi1_div; }
uint16_t BSP_GetFlash2DIV(void) { return mpi2_div; }
uint16_t BSP_GetFlash3DIV(void) { return mpi3_div; }
uint16_t BSP_GetFlash4DIV(void) { return mpi4_div; }
uint16_t BSP_GetFlash5DIV(void) { return mpi5_div; }

void BSP_SetFlash1DIV(uint16_t div) { mpi1_div = div; }
void BSP_SetFlash2DIV(uint16_t div) { mpi2_div = div; }
void BSP_SetFlash3DIV(uint16_t div) { mpi3_div = div; }
void BSP_SetFlash4DIV(uint16_t div) { mpi4_div = div; }
void BSP_SetFlash5DIV(uint16_t div) { mpi5_div = div; }

static uint32_t otp_flash_addr = 0x10000000;

uint32_t BSP_GetOtpBase(void)
{
  return otp_flash_addr;
}

/****************************************************************************
 * Name: BSP_IO_Init / BSP_PIN_Init / BSP_Power_Up
 *
 * Description:
 *   Board-level pinmux and power initialization.
 *   Called by HAL_MspInit() during BSP_Board_PreInit().
 *
 ****************************************************************************/

void BSP_PIN_Init(void)
{
  /* Pinmux for UART1 is handled by arm_earlyserialinit().
   * Additional pinmux can be added here if needed.
   */
}

void BSP_Power_Up(bool bPowerUp)
{
  /* Power up/down for peripherals. Stub for bootloader. */
}

void BSP_IO_Init(void)
{
  BSP_PIN_Init();
  BSP_Power_Up(true);
}

/****************************************************************************
 * Name: BSP_Board_PreInit
 *
 * Description:
 *   Board pre-init for nxboot bootloader.
 *   Configures clocks (XT48, DLL1/2/3), flash clocks, and PSRAM clocks.
 *   Based on the main board's bsp_init.c but simplified for bootloader use.
 *
 ****************************************************************************/

void BSP_Board_PreInit(void)
{
  /* Enable XT48 oscillator */
  HAL_HPAON_EnableXT48();

  /* Select system clock to HXT48 (48 MHz) */
  HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_SYS, RCC_SYSCLK_HXT48);
  HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_HP_PERI, RCC_CLK_PERI_HXT48);

  if (PM_STANDBY_BOOT != SystemPowerOnModeGet())
    {
      /* Halt LCPU first to avoid LCPU in running state */
      HAL_HPAON_WakeCore(CORE_ID_LCPU);
      HAL_RCC_Reset_and_Halt_LCPU(1);

      /* Enable DLL1 at 240 MHz for system clock */
      HAL_PMU_EnableDLL(1);
      HAL_PMU_SWITCH_VRET_LOWER();
      HAL_HPAON_StartGTimer();

      /* Configure LCPU clocks */
      HAL_RCC_LCPU_ClockSelect(RCC_CLK_MOD_LP_PERI, RCC_CLK_PERI_HXT48);
      HAL_LPAON_EnableXT48();
      HAL_RCC_LCPU_ClockSelect(RCC_CLK_MOD_SYS, RCC_SYSCLK_HXT48);
      HAL_RCC_LCPU_SetDiv(1, 1, 3);
    }

  __HAL_SYSCFG_HPBG_EN();
  __HAL_SYSCFG_HPBG_VDDPSW_EN();

  /* Enable DLL1 at 240 MHz and select as system clock */
  HAL_RCC_HCPU_EnableDLL1(240000000);
  HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_SYS, RCC_SYSCLK_DLL1);

  /* Reset sysclk used by HAL_Delay_us */
  HAL_Delay_us(0);

  /* Set flash dividers */
  mpi1_div = 1;
  mpi2_div = 1;
  mpi3_div = 3;
  mpi4_div = 3;
  mpi5_div = 1;

  /* Init the low level hardware (pinmux, power) */
  HAL_MspInit();

  if (mpi3_div <= 4)
    {
      HAL_QSPI_SET_RXDELAY(2, 1, 0);
    }

  /* Enable DLL2 (288 MHz) for PSRAM, DLL3 (216 MHz) for flash */
  HAL_RCC_HCPU_EnableDLL2(288000000);
  HAL_RCC_HCPU_EnableDLL3(216000000);
  HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_FLASH3, RCC_CLK_FLASH_DLL3);
  HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_FLASH4, RCC_CLK_FLASH_DLL3);

#ifdef BSP_USING_PSRAM
  HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_FLASH1, RCC_CLK_FLASH_DLL2);
  HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_FLASH2, RCC_CLK_FLASH_DLL2);
#endif

  /* Set system clock dividers */
  HAL_RCC_HCPU_SetDiv(1, 2, 5);
  HAL_RCC_HCPU_DeepWFIClockSelect(true, RCC_SYSCLK_HXT48);
  HAL_RCC_HCPU_SetDeepWFIDiv(48, 0, 1);
}
