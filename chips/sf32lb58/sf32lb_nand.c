/*
 * SPDX-FileCopyrightText: 2019-2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/****************************************************************************
 * SPI NAND MTD driver for SF32LB58.
 *
 * Wraps the SiFli HAL SPI NAND APIs (HAL_NAND_READ_PAGE,
 * HAL_NAND_WRITE_PAGE, HAL_NAND_ERASE_BLK, etc.) into a NuttX
 * struct mtd_dev_s interface.  Pattern follows sf32lb_flash.c (NOR driver).
 *
 * The NAND flash is connected via MPI3 (QSPI3) and uses the same
 * FLASH_HandleTypeDef as NOR, distinguished by isNand=1.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <debug.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/fs/fs.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/irq.h>
#include <nuttx/kmalloc.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/mutex.h>

#include "bf0_hal_mpi_ex.h"
#include "flash_table.h"
#include "flash_config.h"
#include "sfconfig.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* NAND geometry defaults (overridden by HAL after init) */

#define SF32LB_NAND_DEFAULT_PAGE_SIZE   2048
#define SF32LB_NAND_DEFAULT_BLK_SIZE    (128 * 1024)  /* 64 pages x 2KB */

/* Clock divider for NAND on MPI3 */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct sf32lb_nand_dev_s
{
  struct mtd_dev_s mtd;
  FAR FLASH_HandleTypeDef *handle;
  uint32_t page_size;       /* bytes per page (2048 or 4096) */
  uint32_t block_size;      /* bytes per erase block */
  uint32_t total_size;      /* total flash size in bytes */
  uint32_t nblocks;         /* total erase blocks */
  FAR uint8_t *read_buf;    /* NAND page read cache (data_buf) */
  FAR uint8_t *write_buf;   /* NAND page write cache (data_buf_w) */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static mutex_t g_nand_lock = NXMUTEX_INITIALIZER;

static QSPI_FLASH_CTX_T g_spi_nand_flash_ctx;
static DMA_HandleTypeDef spi_nand_dma_handle;
static bool g_nand_hw_initialized;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int nand_index = -1;   // only ONE nand support in system.
static uint32_t nand_flash_sbus_base;  /* SBUS base for partition offset calc */

static int sf32lb_nand_hw_init(void)
{
  HAL_StatusTypeDef status;

  qspi_configure_t flash_cfg; // = FLASH3_CONFIG;
  struct dma_config flash_dma;

  if (g_nand_hw_initialized)
    {
      return OK;
    }

  memset(&g_spi_nand_flash_ctx, 0, sizeof(g_spi_nand_flash_ctx));
  g_spi_nand_flash_ctx.handle.Instance = FLASH4;
  g_spi_nand_flash_ctx.handle.base = FLASH4_BASE_ADDR;
  g_spi_nand_flash_ctx.handle.size =
    CONFIG_BSP_QSPI4_MEM_SIZE * 1024U * 1024U;
  g_spi_nand_flash_ctx.handle.freq = 24000000;

  struct dma_config flash_dma4 = FLASH4_DMA_CONFIG;
  qspi_configure_t flash_cfg4 = FLASH4_CONFIG;


  BSP_SetFlash4DIV(6);  
  uint16_t div = BSP_GetFlash4DIV(); // 6
  int clk_mode = RCC_CLK_MOD_FLASH4; // 10

  memcpy(&flash_cfg, &flash_cfg4, sizeof(qspi_configure_t));
  memcpy(&flash_dma, &flash_dma4, sizeof(struct dma_config));

  /* Keep flash_cfg.base as CBUS (0x18000000) for the HAL.
   * HAL_FLASH_Init stores cfg->base into both ctx->base_addr and
   * handle->base.  handle->base MUST be CBUS because the HAL's memcpy
   * reads NAND page data from the QSPI memory-mapped region via
   * handle->base — the CPU can only access this at CBUS, not SBUS.
   *
   * Partition offset calculations use SBUS addresses (0x68000000+),
   * so we compute the SBUS base separately.
   */

  nand_flash_sbus_base = HCPU_MPI_SBUS_ADDR(flash_cfg.base);
  
  nand_index = 4;

  g_spi_nand_flash_ctx.handle.freq = flash_get_freq(clk_mode, div, 1);  // 36000000

  status = HAL_FLASH_Init(&g_spi_nand_flash_ctx, &flash_cfg,
                          /* &spi_nand_dma_handle, &flash_dma, */
                          NULL, NULL,
                          div);
  if (status != HAL_OK)
    {
      syslog(LOG_ERR, "ERROR: NAND HAL_FLASH_Init failed: %d\n", status);
      return -EIO;
    }

  /* NOTE: handle.base stays at FLASH4_BASE_ADDR (CBUS 0x18000000).
   * ctx->base_addr is set to SBUS (0x68000000) by HAL_FLASH_Init.
   * Partition offset calculations use ctx->base_addr (SBUS).
   * HAL internal operations use handle.base (CBUS) — do not change.
   */

  /* Allocate NAND page buffer required by HAL */

  g_spi_nand_flash_ctx.handle.data_buf =
    kmm_zalloc(SF32LB_NAND_DEFAULT_PAGE_SIZE);
  if (g_spi_nand_flash_ctx.handle.data_buf == NULL)
    {
      syslog(LOG_ERR, "ERROR: NAND buffer alloc failed\n");
      return -ENOMEM;
    }

  /* Enable ECC */

  HAL_NAND_CONF_ECC(&g_spi_nand_flash_ctx.handle, 1);

  g_nand_hw_initialized = true;
  syslog(LOG_INFO,
    "INFO: SPI NAND initialized on MPI4: page=%lu blk=%lu size=%luMB\n",
    (unsigned long)HAL_NAND_PAGE_SIZE(&g_spi_nand_flash_ctx.handle),
    (unsigned long)HAL_NAND_BLOCK_SIZE(&g_spi_nand_flash_ctx.handle),
    (unsigned long)g_spi_nand_flash_ctx.handle.size / (1024U * 1024U));
  syslog(LOG_INFO,
    "INFO: NAND ctx: dev_id=0x%08lx base_addr=0x%08lx total_size=%lu\n"
    "INFO: NAND handle: base=0x%08lx size=%lu Instance=%p ctable=%p\n"
    "INFO: NAND isNand=%d dma=%p data_buf=%p\n",
    (unsigned long)g_spi_nand_flash_ctx.dev_id,
    (unsigned long)g_spi_nand_flash_ctx.base_addr,
    (unsigned long)g_spi_nand_flash_ctx.total_size,
    (unsigned long)g_spi_nand_flash_ctx.handle.base,
    (unsigned long)g_spi_nand_flash_ctx.handle.size,
    g_spi_nand_flash_ctx.handle.Instance,
    (void *)g_spi_nand_flash_ctx.handle.ctable,
    (int)g_spi_nand_flash_ctx.handle.isNand,
    (void *)g_spi_nand_flash_ctx.handle.dma,
    (void *)g_spi_nand_flash_ctx.handle.data_buf);

  return OK;
}

static int sf32lb_nand_lock(void)
{
  return nxmutex_lock(&g_nand_lock);
}

static int sf32lb_nand_unlock(void)
{
  int ret = nxmutex_unlock(&g_nand_lock);
  return ret < 0 ? ret : OK;
}

/****************************************************************************
 * MTD Callbacks
 ****************************************************************************/

static int sf32lb_nand_geometry(FAR struct sf32lb_nand_dev_s *priv,
                                FAR struct mtd_geometry_s *geo)
{
  if (geo == NULL)
    {
      return -EINVAL;
    }

  memset(geo, 0, sizeof(*geo));
  geo->blocksize = priv->page_size;
  geo->erasesize = priv->block_size;
  geo->neraseblocks = priv->nblocks;
  return OK;
}

static ssize_t sf32lb_nand_read(FAR struct mtd_dev_s *dev, off_t offset,
                                size_t nbytes, FAR uint8_t *buffer)
{
  FAR struct sf32lb_nand_dev_s *priv = (FAR struct sf32lb_nand_dev_s *)dev;
  FAR FLASH_HandleTypeDef *handle = priv->handle;
  uint32_t page_size = priv->page_size;
  size_t remaining = nbytes;
  uint32_t page_addr;
  uint32_t page_off;
  uint32_t chunk;
  int ret;

  if (offset < 0 || buffer == NULL || nbytes == 0)
    {
      return -EINVAL;
    }

  syslog(LOG_INFO,
         "NAND READ entry: offset=0x%08lx nbytes=%lu buf=%p\n",
         (unsigned long)offset, (unsigned long)nbytes, buffer);

  sf32lb_nand_lock();

  while (remaining > 0)
    {
      page_addr = (uint32_t)offset / page_size * page_size;
      page_off = (uint32_t)offset % page_size;
      chunk = page_size - page_off;
      if (chunk > remaining)
        {
          chunk = remaining;
        }

      syslog(LOG_INFO,
             "NAND read: off=0x%08lx page=0x%08lx chunk=%lu "
             "buf=%p ctable=%p isNand=%d\n",
             (unsigned long)offset, (unsigned long)page_addr,
             (unsigned long)chunk, buffer,
             (void *)handle->ctable, (int)handle->isNand);

      if (handle->ctable == NULL)
        {
          syslog(LOG_ERR, "ERROR: NAND ctable is NULL!\n");
          sf32lb_nand_unlock();
          return -EIO;
        }

      ret = HAL_NAND_READ_PAGE(handle, page_addr, buffer, chunk);
      if (ret <= 0)
        {
          syslog(LOG_ERR,
                 "ERROR: NAND read failed: addr=0x%08lx ret=%d\n",
                 (unsigned long)page_addr, ret);
          sf32lb_nand_unlock();
          return -EIO;
        }

      /* Invalidate D-cache so CPU sees DMA'd data */

      SCB_InvalidateDCache_by_Addr(buffer, chunk);

      offset += chunk;
      buffer += chunk;
      remaining -= chunk;
    }

  sf32lb_nand_unlock();
  return (ssize_t)nbytes;
}

static ssize_t sf32lb_nand_bread(FAR struct mtd_dev_s *dev,
                                 off_t startblock, size_t nblocks,
                                 FAR uint8_t *buffer)
{
  FAR struct sf32lb_nand_dev_s *priv = (FAR struct sf32lb_nand_dev_s *)dev;
  FAR FLASH_HandleTypeDef *handle = priv->handle;
  uint32_t page_size = priv->page_size;
  size_t i;
  int ret;

  if (startblock < 0 || buffer == NULL || nblocks == 0)
    {
      return -EINVAL;
    }

  sf32lb_nand_lock();

  if (handle->ctable == NULL)
    {
      syslog(LOG_ERR, "ERROR: NAND ctable is NULL in bread!\n");
      sf32lb_nand_unlock();
      return -EIO;
    }

  for (i = 0; i < nblocks; i++)
    {
      uint32_t page_addr = (uint32_t)(startblock + i) * page_size;

      ret = HAL_NAND_READ_PAGE(handle, page_addr, buffer, page_size);
      if (ret <= 0)
        {
          syslog(LOG_ERR,
                 "ERROR: NAND bread failed: addr=0x%08lx ret=%d\n",
                 (unsigned long)page_addr, ret);
          sf32lb_nand_unlock();
          return -EIO;
        }

      SCB_InvalidateDCache_by_Addr(buffer, page_size);

      buffer += page_size;
    }

  sf32lb_nand_unlock();
  return (ssize_t)nblocks;
}

static ssize_t sf32lb_nand_bwrite(FAR struct mtd_dev_s *dev,
                                  off_t startblock, size_t nblocks,
                                  FAR const uint8_t *buffer)
{
  FAR struct sf32lb_nand_dev_s *priv = (FAR struct sf32lb_nand_dev_s *)dev;
  FAR FLASH_HandleTypeDef *handle = priv->handle;
  uint32_t page_size = priv->page_size;
  size_t i;
  int ret;

  if (startblock < 0 || buffer == NULL || nblocks == 0)
    {
      return -EINVAL;
    }

  sf32lb_nand_lock();

  for (i = 0; i < nblocks; i++)
    {
      uint32_t page_addr = (uint32_t)(startblock + i) * page_size;

      ret = HAL_NAND_WRITE_PAGE(handle, page_addr, buffer, page_size);
      if (ret <= 0)
        {
          syslog(LOG_ERR,
                 "ERROR: NAND bwrite failed: addr=0x%08lx ret=%d\n",
                 (unsigned long)page_addr, ret);
          sf32lb_nand_unlock();
          return -EIO;
        }

      buffer += page_size;
    }

  sf32lb_nand_unlock();
  return (ssize_t)nblocks;
}

static int sf32lb_nand_erase(FAR struct mtd_dev_s *dev, off_t startblock,
                             size_t nblocks)
{
  FAR struct sf32lb_nand_dev_s *priv = (FAR struct sf32lb_nand_dev_s *)dev;
  FAR FLASH_HandleTypeDef *handle = priv->handle;
  uint32_t pages_per_blk = priv->block_size / priv->page_size;
  uint32_t first_eb = (uint32_t)startblock / pages_per_blk;
  uint32_t last_eb = ((uint32_t)(startblock + nblocks) - 1) / pages_per_blk;
  uint32_t eb;
  int ret;

  if (startblock < 0 || nblocks == 0)
    {
      return -EINVAL;
    }

  sf32lb_nand_lock();

  for (eb = first_eb; eb <= last_eb; eb++)
    {
      uint32_t blk_addr = eb * priv->block_size;

      ret = HAL_NAND_ERASE_BLK(handle, blk_addr);
      if (ret != 0)
        {
          syslog(LOG_ERR,
                 "ERROR: NAND erase failed: eb=%lu addr=0x%08lx ret=%d\n",
                 (unsigned long)eb,
                 (unsigned long)blk_addr, ret);
          sf32lb_nand_unlock();
          return -EIO;
        }
    }

  sf32lb_nand_unlock();
  return (int)nblocks;
}

static int sf32lb_nand_ioctl(FAR struct mtd_dev_s *dev, int cmd,
                             unsigned long arg)
{
  FAR struct sf32lb_nand_dev_s *priv = (FAR struct sf32lb_nand_dev_s *)dev;
  int ret = -EINVAL;

  switch (cmd)
    {
      case MTDIOC_GEOMETRY:
        {
          FAR struct mtd_geometry_s *geo =
            (FAR struct mtd_geometry_s *)((uintptr_t)arg);

          if (geo != NULL)
            {
              ret = sf32lb_nand_geometry(priv, geo);
            }
        }
        break;

      case BIOC_PARTINFO:
        {
          FAR struct partition_info_s *info =
            (FAR struct partition_info_s *)arg;

          if (info != NULL)
            {
              info->numsectors = priv->nblocks *
                                (priv->block_size / priv->page_size);
              info->sectorsize = priv->page_size;
              info->startsector = 0;
              info->parent[0] = '\0';
              ret = OK;
            }
        }
        break;

      case MTDIOC_ERASESTATE:
        {
          FAR uint8_t *result = (FAR uint8_t *)arg;
          *result = 0xff;
          ret = OK;
        }
        break;

      case MTDIOC_BULKERASE:
        {
          ret = sf32lb_nand_erase(dev, 0, priv->nblocks) ==
                (int)priv->nblocks ? OK : -EIO;
        }
        break;

      default:
        ret = -ENOTTY;
        break;
    }

  return ret;
}

static int sf32lb_nand_isbad(FAR struct mtd_dev_s *dev, off_t block)
{
  FAR struct sf32lb_nand_dev_s *priv = (FAR struct sf32lb_nand_dev_s *)dev;
  int ret;

  ret = HAL_NAND_GET_BADBLK(priv->handle, (uint32_t)block);
  return ret;  /* 0 = good, nonzero = bad */
}

static int sf32lb_nand_markbad(FAR struct mtd_dev_s *dev, off_t block)
{
  FAR struct sf32lb_nand_dev_s *priv = (FAR struct sf32lb_nand_dev_s *)dev;
  int ret;

  ret = HAL_NAND_MARK_BADBLK(priv->handle, (uint32_t)block, 1);
  return ret == 0 ? OK : -EIO;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sf32lb_nand_initialize
 *
 * Description:
 *   Initialize the SPI NAND flash on MPI3 and return an MTD device.
 *
 ****************************************************************************/

FAR struct mtd_dev_s *sf32lb_nand_initialize(void)
{
  FAR struct sf32lb_nand_dev_s *priv;
  int ret;

  priv = kmm_zalloc(sizeof(struct sf32lb_nand_dev_s));
  if (priv == NULL)
    {
      return NULL;
    }

  ret = sf32lb_nand_hw_init();
  if (ret < 0)
    {
      kmm_free(priv);
      return NULL;
    }

  priv->handle = &g_spi_nand_flash_ctx.handle;
  priv->page_size = HAL_NAND_PAGE_SIZE(priv->handle);
  priv->block_size = HAL_NAND_BLOCK_SIZE(priv->handle);
  priv->total_size = priv->handle->size;
  priv->nblocks = priv->total_size / priv->block_size;
  priv->read_buf = priv->handle->data_buf;
  priv->write_buf = kmm_zalloc(priv->page_size);
  if (priv->write_buf == NULL)
    {
      kmm_free(priv);
      return -ENOMEM;
    }

  priv->mtd.erase   = sf32lb_nand_erase;
  priv->mtd.bread   = sf32lb_nand_bread;
  priv->mtd.bwrite  = sf32lb_nand_bwrite;
  priv->mtd.read    = sf32lb_nand_read;
  priv->mtd.ioctl   = sf32lb_nand_ioctl;
  priv->mtd.isbad   = sf32lb_nand_isbad;
  priv->mtd.markbad = sf32lb_nand_markbad;
  priv->mtd.name    = "nand";

  syslog(LOG_INFO,
    "INFO: NAND MTD: page=%lu blk=%lu nblk=%lu total=%luMB\n",
    (unsigned long)priv->page_size,
    (unsigned long)priv->block_size,
    (unsigned long)priv->nblocks,
    (unsigned long)priv->total_size / (1024U * 1024U));

  return (FAR struct mtd_dev_s *)priv;
}

/****************************************************************************
 * Name: sf32lb_nand_register_partitions
 *
 * Description:
 *   Register NAND partitions as MTD character devices.
 *   Each partition is a sub-region of the NAND flash.
 *
 *   part_offsets[i] and part_sizes[i] are in bytes.
 *   A size of 0 means "use remaining flash".
 *
 ****************************************************************************/

int sf32lb_nand_register_partitions(
  FAR const char *const *devnames,
  FAR const uint32_t *part_offsets,
  FAR const uint32_t *part_sizes,
  int npartitions)
{
  FAR struct mtd_dev_s *whole;
  FAR struct mtd_dev_s *part;
  int ret;
  int i;

  whole = sf32lb_nand_initialize();
  if (whole == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize NAND flash\n");
      return -ENODEV;
    }

  for (i = 0; i < npartitions; i++)
    {
      if (devnames[i] == NULL)
        {
          continue;
        }

      if (part_sizes[i] == 0)
        {
          /* Single partition covering the whole device */

          part = whole;
        }
      else
        {
          /* Create a sub-MTD for this partition.
           * part_offsets[] are absolute SBUS addresses — convert to
           * block numbers relative to the start of the NAND device.
           */

          FAR struct sf32lb_nand_dev_s *wdev =
            (FAR struct sf32lb_nand_dev_s *)whole;
          uint32_t page_size = wdev->page_size;
          off_t startblock =
            (off_t)((part_offsets[i] - nand_flash_sbus_base) / page_size);
          size_t nblocks = (size_t)(part_sizes[i] / page_size);

          part = mtd_partition(whole, startblock, nblocks);
          if (part == NULL)
            {
              syslog(LOG_ERR,
                     "ERROR: mtd_partition(%s, pg=%lu, n=%lu) failed\n",
                     devnames[i],
                     (unsigned long)startblock,
                     (unsigned long)nblocks);
              continue;
            }
        }

      ret = register_mtddriver(devnames[i], part, 0755, part);
      if (ret < 0)
        {
          syslog(LOG_ERR,
                 "ERROR: register_mtddriver(%s) failed: %d\n",
                 devnames[i], ret);
        }
      else
        {
          syslog(LOG_INFO,
                 "INFO: NAND partition %s registered (offset=0x%08lx "
                 "size=0x%08lx)\n",
                 devnames[i],
                 (unsigned long)part_offsets[i],
                 (unsigned long)part_sizes[i]);
        }
    }

  return OK;
}
