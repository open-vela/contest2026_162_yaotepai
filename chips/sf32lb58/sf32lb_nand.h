/****************************************************************************
 * chips/sf32lb58/sf32lb_nand.h
 *
 * Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPI NAND MTD driver for SF32LB58.
 ****************************************************************************/

#ifndef __CHIPS_SF32LB58_SF32LB_NAND_H
#define __CHIPS_SF32LB58_SF32LB_NAND_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/mtd/mtd.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: sf32lb_nand_initialize
 *
 * Description:
 *   Initialize the SPI NAND flash on MPI3 and return an MTD device.
 *
 * Returned Value:
 *   MTD device pointer on success, NULL on failure.
 *
 ****************************************************************************/

FAR struct mtd_dev_s *sf32lb_nand_initialize(void);

/****************************************************************************
 * Name: sf32lb_nand_register_partitions
 *
 * Description:
 *   Register NAND partitions as MTD character devices.
 *
 * Input Parameters:
 *   devnames    - Array of device path names (e.g. "/dev/ota0")
 *   part_offsets - Array of partition offsets in bytes
 *   part_sizes   - Array of partition sizes in bytes (0 = whole device)
 *   npartitions  - Number of partitions
 *
 * Returned Value:
 *   OK on success, negative errno on failure.
 *
 ****************************************************************************/

int sf32lb_nand_register_partitions(
  FAR const char *const *devnames,
  FAR const uint32_t *part_offsets,
  FAR const uint32_t *part_sizes,
  int npartitions);

#endif /* __CHIPS_SF32LB58_SF32LB_NAND_H */
