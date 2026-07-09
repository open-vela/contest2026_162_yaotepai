# ##############################################################################
# board/sf32lb58-lcd_a128r32n1_qspi/drivers/CMakeLists.txt
#
# Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
# SPDX-License-Identifier: Apache-2.0
# ##############################################################################

set(VENDOR_DIR "${NUTTX_DIR}/../vendor/sifli/chips")

target_compile_definitions(drivers PRIVATE SOC_BF0_HCPU
                                           SF32LB58X
                                           HAL_TICK_PER_SECOND=1000
                                           USE_HAL_DRIVER)

target_include_directories(drivers PRIVATE ${NUTTX_CHIP_ABS_DIR}
                                           ${NUTTX_CHIP_ABS_DIR}/include
                                           ${VENDOR_DIR}/drivers/Include
                                           ${VENDOR_DIR}/drivers/cmsis/Include
                                           ${VENDOR_DIR}/drivers/cmsis/sf32lb58x
                                           ${VENDOR_DIR}/external/CMSIS/Include
                                           ${VENDOR_DIR}/boards/include/config/sf32lb58x
                                           ${NUTTX_BOARD_ABS_DIR}/include
                                           ${NUTTX_DIR}/arch/arm/src/common
                                           ${NUTTX_DIR}/arch/arm/src/armv8-m
                                           ${NUTTX_DIR}/arch/arm/src/arm_m)

nuttx_add_subdirectory()
