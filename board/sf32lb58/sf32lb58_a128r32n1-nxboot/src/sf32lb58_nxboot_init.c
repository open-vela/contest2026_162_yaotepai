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
#include "sfconfig.h"
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
/****************************************************************************
 * Public Functions
 ****************************************************************************/
__STATIC_INLINE void dwtIpInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

__STATIC_INLINE void dwtIpDeinit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
}

__STATIC_INLINE void dwtReset(void)
{
    DWT->CYCCNT = 0; /* Clear DWT cycle counter */
}

__STATIC_INLINE uint32_t dwtGetCycles(void)
{
    return DWT->CYCCNT;
}

static void boot_flash_power_on(void)
{
#define V58_PIN (58U)
    uint8_t pin = V58_PIN;

#if 0
    //GPIO_InitTypeDef gpio_config;
    //GPIO_A39
    gpio_config.Pin = 39;
    gpio_config.Mode = GPIO_MODE_OUTPUT;
    gpio_config.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(hwp_gpio1, &gpio_config);
    HAL_GPIO_WritePin(hwp_gpio1, 39, GPIO_PIN_SET);
#endif

    /* enable V43 */
    pin = 43 - 32;
    (hwp_gpio1 + 1)->DOESR |= (1UL << pin);
    (hwp_gpio1 + 1)->DOR |= (1UL << pin);

    /* enable V33 */
    pin = 33 - 32;
    (hwp_gpio1 + 1)->DOESR |= (1UL << pin);
    (hwp_gpio1 + 1)->DOR |= (1UL << pin);

    /* enable V26 */
    pin = 26;
    (hwp_gpio1 + 0)->DOESR |= (1UL << pin);
    (hwp_gpio1 + 0)->DOR |= (1UL << pin);

#ifdef SOC_BF0_HCPU
    // Enable PADA
    HAL_HPAON_ENABLE_PAD();

    HAL_PBR0_FORCE1_ENABLE();
    HAL_PBR_ConfigMode(0, 1); //set PBR0 output
    HAL_PBR_WritePin(0, 1); //set PBR0 high

#endif /* SOC_BF0_HCPU */

    dwtIpInit();
    /* wait until 2ms elapse to ensure flash LDO is stable before flash access
     * default clock is 24MHz, maybe slower than 24MHz at startup stage
     *
     */
    while (dwtGetCycles() < (2 * 24000))
    {
    }

    dwtIpDeinit();

#if defined(SOC_SF32LB58X) && defined(FPGA)
    HAL_QSPI_SET_RXDELAY(0, 0, 2);
    HAL_QSPI_SET_RXDELAY(1, 0, 2);
#endif
}

static void config_psram_pinmux(void)
{
    if (CHIP_IS_583())
    {
        // PSRAM1
        HAL_PIN_Set(PAD_SA04, MPI1_DIO0, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA05, MPI1_DIO1, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA06, MPI1_DIO2, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA07, MPI1_DIO3, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA09, MPI1_CS,  PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SA10, MPI1_CLK,   PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SA11, MPI1_DIO4, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA12, MPI1_DIO5, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA13, MPI1_DIO6, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA14, MPI1_DIO7, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA15, MPI1_DQS0,  PIN_PULLDOWN, 1);

        // PSRAM2
        HAL_PIN_Set(PAD_SB04, MPI2_DIO0, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB05, MPI2_DIO1, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB06, MPI2_DIO2, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB07, MPI2_DIO3, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB09, MPI2_CS,  PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SB10, MPI2_CLK,   PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SB11, MPI2_DIO4, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB12, MPI2_DIO5, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB13, MPI2_DIO6, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB14, MPI2_DIO7, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB15, MPI2_DQS0,  PIN_PULLDOWN, 1);
    }
    else if (CHIP_IS_587())
    {
        // PSRAM1
        HAL_PIN_Set(PAD_SA00, MPI1_DIO0, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA01, MPI1_DIO1, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA02, MPI1_DIO2, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA03, MPI1_DIO3, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA04, MPI1_DIO4, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA05, MPI1_DIO5, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA06, MPI1_DIO6, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA07, MPI1_DIO7, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA08, MPI1_DQS0, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA09, MPI1_CLK,   PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SA10, MPI1_CS,  PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SA11, MPI1_DIO8, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA12, MPI1_DIO9, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA13, MPI1_DIO10, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA14, MPI1_DIO11, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA15, MPI1_DIO12, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA16, MPI1_DIO13, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA17, MPI1_DIO14, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA18, MPI1_DIO15, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA19, MPI1_DQS1, PIN_PULLDOWN, 1);

        // PSRAM2
        HAL_PIN_Set(PAD_SB00, MPI2_DIO0, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB01, MPI2_DIO1, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB02, MPI2_DIO2, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB03, MPI2_DIO3, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB04, MPI2_DIO4, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB05, MPI2_DIO5, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB06, MPI2_DIO6, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB07, MPI2_DIO7, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB08, MPI2_DQS0, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB09, MPI2_CLK,   PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SB10, MPI2_CS,  PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SB11, MPI2_DIO8, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB12, MPI2_DIO9, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB13, MPI2_DIO10, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB14, MPI2_DIO11, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB15, MPI2_DIO12, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB16, MPI2_DIO13, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB17, MPI2_DIO14, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB18, MPI2_DIO15, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB19, MPI2_DQS1, PIN_PULLDOWN, 1);
    }
    else if (CHIP_IS_585())
    {
        // PSRAM2
        HAL_PIN_Set(PAD_SB00, MPI2_DIO0, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB01, MPI2_DIO1, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB02, MPI2_DIO2, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB03, MPI2_DIO3, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB04, MPI2_DIO4, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB05, MPI2_DIO5, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB06, MPI2_DIO6, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB07, MPI2_DIO7, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB08, MPI2_DQS0, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB09, MPI2_CLK,   PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SB10, MPI2_CS,  PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SB11, MPI2_DIO8, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB12, MPI2_DIO9, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB13, MPI2_DIO10, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB14, MPI2_DIO11, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB15, MPI2_DIO12, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB16, MPI2_DIO13, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB17, MPI2_DIO14, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB18, MPI2_DIO15, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SB19, MPI2_DQS1, PIN_PULLDOWN, 1);
    }
}

void BSP_PIN_Init(void)
{
#ifdef SOC_BF0_HCPU
    // HCPU pins
    config_psram_pinmux();

    HAL_PIN_Set(PAD_PA16, GPIO_A16, PIN_NOPULL, 1);//TP rst
    HAL_PIN_Set(PAD_PA17, GPIO_A17, PIN_NOPULL, 1);//TP irq
    HAL_PIN_Set(PAD_PA18, GPIO_A18, PIN_PULLDOWN, 1);//LCD rst

    // I2C1 for touch panel
    HAL_PIN_Set(PAD_PA59, I2C4_SDA, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA60, I2C4_SCL, PIN_PULLUP, 1);
    // I2C3 for PA
    HAL_PIN_Set(PAD_PA93, I2C3_SDA, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA92, I2C3_SCL, PIN_PULLUP, 1);

    //PA
    HAL_PIN_Set(PAD_PA88, GPIO_A88, PIN_PULLDOWN, 1);

#ifdef BSP_ENABLE_MPI4
    HAL_PIN_Set(PAD_PA39, MPI4_CLK, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA30, MPI4_CS, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA40, MPI4_DIO0, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_PA37, MPI4_DIO1, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_PA36, MPI4_DIO2, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA38, MPI4_DIO3, PIN_PULLUP, 1);
#endif
#ifdef BSP_ENABLE_MPI3
    //TODO: power pin
    //BSP_GPIO_Set(MPI1_POWER_PIN, 1, 1);
    //BSP_GPIO_Set(MPI2_POWER_PIN, 1, 1);
    //BSP_GPIO_Set(MPI3_POWER_PIN, 1, 1);

    // MPI3
    HAL_PIN_Set(PAD_PA46, MPI3_CLK, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA44, MPI3_CS, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA50, MPI3_DIO0, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_PA48, MPI3_DIO1, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_PA47, MPI3_DIO2, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA45, MPI3_DIO3, PIN_PULLUP, 1);

#endif

    // UART1
    HAL_PIN_Set(PAD_PA31, USART1_TXD, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA32, USART1_RXD, PIN_PULLUP, 1);

    // I2S2
#if defined(BSP_ENABLE_I2S_CODEC)
    HAL_PIN_Set(PAD_PA82, I2S2_SDO, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA91, I2S2_BCK, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA84, I2S2_LRCK, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA86, I2S2_SDI, PIN_PULLDOWN, 1);
#endif
    // I2S3
#if defined(BSP_ENABLE_I2S3)
    HAL_PIN_Set(PAD_PB24, I2S3_SDO, PIN_NOPULL, 0);
    HAL_PIN_Set(PAD_PB30, I2S3_BCK, PIN_NOPULL, 0);
    HAL_PIN_Set(PAD_PB31, I2S3_LRCK, PIN_NOPULL, 0);
    HAL_PIN_Set(PAD_PB27, I2S3_SDI, PIN_PULLDOWN, 0);
#endif

#ifdef BSP_USING_PDM1
    // PDM1
    HAL_PIN_Set(PAD_PA24, GPIO_A24, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA23, PDM1_CLK, PIN_NOPULL, 1);
    //HAL_PIN_Set(PAD_PA18, PDM1_DATA, PIN_PULLDOWN, 1);
    //I2S1
#elif defined(BSP_ENABLE_I2S_MIC)
    HAL_PIN_Set(PAD_PA14, I2S1_LRCK, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA23, I2S1_BCK, PIN_NOPULL, 1);
    //HAL_PIN_Set(PAD_PA18, I2S1_SDI, PIN_PULLDOWN, 1);
#endif

    // UART2
    HAL_PIN_Set(PAD_PA28, USART2_TXD, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA29, USART2_RXD, PIN_PULLUP, 1);

    // LCDC1
#if 0//def BSP_LCDC_USING_DBI
    HAL_PIN_Set(PAD_PA20, LCDC1_8080_WR, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA31, LCDC1_8080_CS, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA34, LCDC1_8080_RD, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA36, LCDC1_8080_DC, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA38, LCDC1_8080_DIO0, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA42, LCDC1_8080_DIO1, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA44, LCDC1_8080_DIO2, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA45, LCDC1_8080_DIO3, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA47, LCDC1_8080_DIO4, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA49, LCDC1_8080_DIO5, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA51, LCDC1_8080_DIO6, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA55, LCDC1_8080_DIO7, PIN_PULLUP, 1);

    HAL_PIN_Set(PAD_PA77, LCDC1_8080_TE, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA78, LCDC1_8080_RSTB, PIN_PULLUP, 1);
#elif 0//defined(BSP_LCDC_USING_JDI_PARALLEL) Always use LPSYS JDI
    HAL_PIN_Set(PAD_PA19, LCDC1_JDI_VCK, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA22, LCDC1_JDI_VST, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA25, LCDC1_JDI_XRST, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA43, LCDC1_JDI_HCK, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA44, LCDC1_JDI_HST, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA45, LCDC1_JDI_ENB, PIN_PULLUP, 1);
    //HAL_PIN_Set(PAD_PA42, LCDC1_JDI_DIO1, PIN_PULLUP, 1);
    //HAL_PIN_Set(PAD_PA44, LCDC1_JDI_FRP, PIN_PULLUP, 1);
    //HAL_PIN_Set(PAD_PA45, LCDC1_JDI_XFRP, PIN_PULLUP, 1);
    //HAL_PIN_Set(PAD_PA47, LCDC1_JDI_VCOM, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA46, LCDC1_JDI_R1, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA47, LCDC1_JDI_R2, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA48, LCDC1_JDI_G1, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA50, LCDC1_JDI_G2, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA65, LCDC1_JDI_B1, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA67, LCDC1_JDI_B2, PIN_PULLUP, 1);
#elif defined(BSP_LCDC_USING_DPI)
    HAL_PIN_Set(PAD_PA12, LCDC1_DPI_CLK,    PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA13, LCDC1_DPI_DE,     PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA14, LCDC1_DPI_HSYNC,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA15, LCDC1_DPI_VSYNC,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA22, LCDC1_DPI_R0,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA23, LCDC1_DPI_R1,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA24, LCDC1_DPI_R2,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA25, LCDC1_DPI_R3,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA26, LCDC1_DPI_R4,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA27, LCDC1_DPI_R5,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA43, LCDC1_DPI_R6,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA44, LCDC1_DPI_R7,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA45, LCDC1_DPI_G0,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA46, LCDC1_DPI_G1,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA47, LCDC1_DPI_G2,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA48, LCDC1_DPI_G3,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA50, LCDC1_DPI_G4,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA53, LCDC1_DPI_G5,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA54, LCDC1_DPI_G6,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA55, LCDC1_DPI_G7,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA56, LCDC1_DPI_B0,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA57, LCDC1_DPI_B1,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA58, LCDC1_DPI_B2,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA61, LCDC1_DPI_B3,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA62, LCDC1_DPI_B4,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA63, LCDC1_DPI_B5,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA65, LCDC1_DPI_B6,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA67, LCDC1_DPI_B7,  PIN_NOPULL, 1);
    //HAL_PIN_Set(PAD_PA67, LCDC1_DPI_CM,  PIN_NOPULL, 1);
#elif defined(BSP_LCDC_USING_QADSPI)
    HAL_PIN_Set(PAD_PA46, LCDC1_SPI_CLK, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA44, LCDC1_SPI_CS, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA50, LCDC1_SPI_DIO0, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA48, LCDC1_SPI_DIO1, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA47, LCDC1_SPI_DIO2, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA45, LCDC1_SPI_DIO3, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA43, LCDC1_SPI_TE, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA18, GPIO_A18, PIN_NOPULL, 1); //LCD rst
#endif

    HAL_PIN_Set(PAD_PA42, GPTIM2_CH4, PIN_NOPULL, 1); //LCD backlight PWM( DSI&DPI LCD)

    /*SDHCI1 config*/
#ifdef BSP_USING_SDHCI1
    BSP_sd1_pinmux_config();
#endif

    /*SDHCI2 config*/
#ifdef BSP_USING_SDHCI2
    BSP_sd2_pinmux_config();
#endif
    if (PM_STANDBY_BOOT != SystemPowerOnModeGet())
    {
        HAL_PBR0_FORCE1_ENABLE();
    }
#endif

    // LCPU pins
    HAL_PIN_Set(PAD_PB14, USART6_TXD, PIN_NOPULL, 0);
    HAL_PIN_Set(PAD_PB13, USART6_RXD, PIN_PULLUP, 0);

    // PTC debug pin
    HAL_PIN_Set(PAD_PB18, GPIO_B18, PIN_NOPULL, 0);

    // PA CTRL PIN
    HAL_PIN_Set(PAD_PB23, GPIO_B23, PIN_NOPULL, 0);

    HAL_PIN_Set(PAD_PB32, ANA_PIN_FUNC, PIN_NOPULL, 0); // AUD RESET USED AS ADC 0 , remove it if audio i2s3 use reset
    HAL_PIN_Set(PAD_PB34, ANA_PIN_FUNC, PIN_NOPULL, 0); // AUD MCLK, USED AS GPADC CH2
    HAL_PIN_Set(PAD_PB33, ANA_PIN_FUNC, PIN_NOPULL, 0); // NTC GPADC CH1
    HAL_PIN_Set(PAD_PB35, ANA_PIN_FUNC, PIN_NOPULL, 0); // VBAT GPADC CH3
#if defined(SOC_BF0_LCPU)
    HAL_PIN_Set_Analog(PAD_PB32, 0);
    HAL_PIN_Set_Analog(PAD_PB33, 0);
    HAL_PIN_Set_Analog(PAD_PB34, 0);
    HAL_PIN_Set_Analog(PAD_PB35, 0);

#endif

    // Key1
    HAL_PIN_Set(PAD_PB54, GPIO_B43, PIN_NOPULL, 0);

    HAL_PIN_Set(PAD_PB37, USART4_TXD, PIN_NOPULL, 0);
    HAL_PIN_Set(PAD_PB36, USART4_RXD, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB18, USART5_TXD, PIN_NOPULL, 0);
    HAL_PIN_Set(PAD_PB17, USART5_RXD, PIN_PULLUP, 0);

    // I2C5
    HAL_PIN_Set(PAD_PB48, I2C5_SCL, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB47, I2C5_SDA, PIN_PULLUP, 0);

    // I2C6
    HAL_PIN_Set(PAD_PB28, I2C6_SCL, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB29, I2C6_SDA, PIN_PULLUP, 0);

#if 0
    //TODO:
    HAL_PIN_Set(PAD_PB12, SPI4_CLK, PIN_NOPULL, 0);
    HAL_PIN_Set(PAD_PB19, SPI4_CS, PIN_NOPULL, 0);
    HAL_PIN_Set(PAD_PB15, SPI4_DI, PIN_PULLDOWN, 0);
    HAL_PIN_Set(PAD_PB16, SPI4_DO, PIN_NOPULL, 0);
#endif

#if defined(BSP_LCDC_USING_JDI_PARALLEL)
    HAL_PIN_Set(PAD_PB15, LCDC2_JDI_VCK, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB19, LCDC2_JDI_VST, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB16, LCDC2_JDI_XRST, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB05, LCDC2_JDI_HCK, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB10, LCDC2_JDI_HST, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB12, LCDC2_JDI_ENB, PIN_PULLUP, 0);
    //HAL_PIN_Set(PAD_PB42, LCDC2_JDI_DIO1, PIN_PULLUP, 0);
    //HAL_PIN_Set(PAD_PB44, LCDC2_JDI_FRP, PIN_PULLUP, 0);
    //HAL_PIN_Set(PAD_PB45, LCDC2_JDI_XFRP, PIN_PULLUP, 0);
    //HAL_PIN_Set(PAD_PB47, LCDC2_JDI_VCOM, PIN_PULLUP, 0);
    /*
        lcdc_jdi_frp  ---- PBR3
        lcdc_jdi_xfrp ---- PBR4
        lcdc_jdi_vcom ---- PBR5
    */
    HAL_PIN_Set(PAD_PB09, LCDC2_JDI_R1, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB06, LCDC2_JDI_R2, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB08, LCDC2_JDI_G1, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB04, LCDC2_JDI_G2, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB02, LCDC2_JDI_B1, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB03, LCDC2_JDI_B2, PIN_PULLUP, 0);
#else
    /*LCD SPI*/
    HAL_PIN_Set(PAD_PB02, LCDC2_SPI_TE,   PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB03, LCDC2_SPI_DIO1, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB04, LCDC2_SPI_DIO2, PIN_PULLUP, 0);
    //HAL_PIN_Set(PAD_PB05, LCDC2_SPI_RSTB, PIN_PULLUP, 0); Use default gpio mode
    HAL_PIN_Set(PAD_PB06, LCDC2_SPI_DIO3, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB08, LCDC2_SPI_CS,   PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB09, LCDC2_SPI_DIO0, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB10, LCDC2_SPI_CLK,  PIN_PULLUP, 0);
#endif

#if !defined(BSP_LCDC_USING_JDI_PARALLEL)
    HAL_PIN_Set(PAD_PB12, SPI4_CLK, PIN_PULLDOWN, 0);             // SPI4 (GSensor)
    HAL_PIN_Set(PAD_PB15, SPI4_DI, PIN_PULLDOWN, 0);
    HAL_PIN_Set(PAD_PB16, SPI4_DO, PIN_PULLDOWN, 0);
    HAL_PIN_Set(PAD_PB19, SPI4_CS, PIN_PULLUP, 0);
#endif
    HAL_PIN_Set(PAD_PB59, GPIO_B59, PIN_NOPULL, 0);             // SPI4_EN

    HAL_PIN_Set(PAD_PB28, I2C6_SCL, PIN_PULLUP, 0);             // I2C6 (Heart rate sensor)
    HAL_PIN_Set(PAD_PB29, I2C6_SDA, PIN_PULLUP, 0);
    HAL_PIN_Set(PAD_PB39, GPIO_B39, PIN_NOPULL, 0);              // Heart power
    HAL_PIN_Set(PAD_PB57, GPIO_B57, PIN_NOPULL, 0);              // hrt INT
    HAL_PIN_Set(PAD_PB53, GPIO_B53, PIN_PULLUP, 0);     //BSP_CHARGER_EN_PIN
    HAL_PIN_Set(PAD_PB47, GPIO_B47, PIN_PULLUP, 0);     //BSP_CHARGE_FULL_PIN
    HAL_PIN_Set(PAD_PB48, GPIO_B48, PIN_PULLUP, 0);     //BSP_CHARGING_PIN
    HAL_PIN_Set(PAD_PB56, GPIO_B56, PIN_PULLDOWN, 0);     //BSP_CHARGER_INT_PIN   INT

    /* Pinmux for UART1 is handled by arm_earlyserialinit().
   * Additional pinmux can be added here if needed.
   */
}

void BSP_Power_Up(bool bPowerUp)
{
  /* Power up/down for peripherals. Stub for bootloader. */
    boot_flash_power_on();  
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
