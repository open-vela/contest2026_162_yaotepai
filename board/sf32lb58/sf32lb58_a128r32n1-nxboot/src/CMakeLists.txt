# ##############################################################################
# board/sf32lb58/sf32lb58-nxboot/src/CMakeLists.txt
#
# Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
# SPDX-License-Identifier: Apache-2.0
#
# ##############################################################################

set(SRCS sf32lb58_boot_bringup.c board_boot_image.c
         sf32lb58_nxboot_init.c)

target_sources(board PRIVATE ${SRCS})

set(VENDOR_DIR "${NUTTX_DIR}/../vendor/sifli/chips")

target_compile_definitions(board PRIVATE SOC_BF0_HCPU
                                         SF32LB58X
                                         HAL_TICK_PER_SECOND=1000
                                         USE_HAL_DRIVER)

target_include_directories(board PRIVATE ${NUTTX_CHIP_ABS_DIR}/include)
target_include_directories(board PRIVATE ${VENDOR_DIR}/drivers/Include)
target_include_directories(board PRIVATE ${VENDOR_DIR}/drivers/cmsis/Include)
target_include_directories(board PRIVATE ${VENDOR_DIR}/drivers/cmsis/sf32lb58x)
target_include_directories(board PRIVATE ${VENDOR_DIR}/external/CMSIS/Include)
target_include_directories(board PRIVATE ${VENDOR_DIR}/boards/include/config/sf32lb58x)

target_compile_options(board PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-w>
    $<$<COMPILE_LANGUAGE:CXX>:-w>)
