/*
 * SPDX-FileCopyrightText: 2019-2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __VENDOR_SIFLI_SF32LB52_SF32LB_FLASH_H
#define __VENDOR_SIFLI_SF32LB52_SF32LB_FLASH_H

#include "bf0_hal_mpi.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int sf32lb_flash_lock(void);
int sf32lb_flash_unlock(void);
int sf32lb_nor_automount(int minor, int block_offset, int block_count);
int sf32lb_flash_register_partitions(FAR const char *const *devnames,
                                     FAR const uint32_t *part_offsets,
                                     FAR const uint32_t *part_sizes,
                                     int npartitions);
FAR struct mtd_dev_s *sf32lb_flash_mtd_initialize(void);

/* Get the NOR flash HAL handle for MPI2 (QSPI2) */
FLASH_HandleTypeDef *sf32lb_flash_get_handle(void);

#endif /* __VENDOR_SIFLI_SF32LB52_SF32LB_FLASH_H */
