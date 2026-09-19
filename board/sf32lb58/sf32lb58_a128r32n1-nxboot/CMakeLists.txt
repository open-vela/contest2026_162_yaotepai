# ##############################################################################
# board/sf32lb58/sf32lb58-nxboot/CMakeLists.txt
#
# Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
# SPDX-License-Identifier: Apache-2.0
#
# ##############################################################################

add_subdirectory(src)

file(GLOB EXTRA_LIBS
     ${NUTTX_BOARD_ABS_DIR}/libs/${CONFIG_ARCH_BOARD_CUSTOM_NAME}/*.a)

set_property(GLOBAL APPEND PROPERTY NUTTX_EXTRA_LIBRARIES ${EXTRA_LIBS})

set_property(GLOBAL PROPERTY LD_SCRIPT ${NUTTX_BOARD_ABS_DIR}/scripts/ld.script)

# Allow our _vectors override in sifli_start.c to supersede the NuttX default
# in arm_vectors.c.  The ROM bootloader does not set MSP from the vector
# table, so we need a custom reset handler that sets MSP before any C code.
target_link_options(nuttx PRIVATE -Wl,-z,muldefs)
