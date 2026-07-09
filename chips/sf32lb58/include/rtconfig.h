/****************************************************************************
 * chips/sf32lb58/include/rtconfig.h
 *
 * Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Compatibility shim: vendor HAL config headers include <rtconfig.h>
 * expecting RT-Thread symbols.  In openvela/NuttX we include nuttx/config.h
 * (which defines CONFIG_BSP_USING_* etc.) and then sfconfig.h which
 * translates them to the BSP_USING_* macros the HAL expects.
 ****************************************************************************/

#ifndef __CHIPS_SF32LB58_INCLUDE_RTCONFIG_H
#define __CHIPS_SF32LB58_INCLUDE_RTCONFIG_H

#include <nuttx/config.h>
#include "sfconfig.h"

#endif /* __CHIPS_SF32LB58_INCLUDE_RTCONFIG_H */
