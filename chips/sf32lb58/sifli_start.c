/****************************************************************************
 * vendor/sifli/chip/sf32lb52/sifli_start.c
 *
 * Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdarg.h>
#include <stdio.h>

#include <nuttx/arch.h>
#include <nuttx/init.h>
#include <nuttx/cache.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "system_bf0_ap.h"
/* define __PROGRAM_START is to exclude define to __cmsis_start */
#define __PROGRAM_START __start
#include "bf0_hal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* .data is positioned first in the primary RAM followed immediately by .bss.
 * The IDLE thread stack lies just after .bss and has size give by
 * CONFIG_IDLETHREAD_STACKSIZE;  The heap then begins just after the IDLE.
 * ARM EABI requires 64 bit stack alignment.
 */

#define HEAP_BASE      ((uintptr_t)_ebss + CONFIG_IDLETHREAD_STACKSIZE)

extern uint32_t _siramfunc;
extern uint32_t _sramfunc;
extern uint32_t _eramfunc;

void arm_lowputs(const char *str)
{
  while (*str)
    {
      arm_lowputc(*str++);
    }
}

/* Early printf implementation */
int arm_lowprintf(const char *fmt, ...)
{
  va_list ap;
  char buf[256];
  int ret;

  /* Format the message */
  va_start(ap, fmt);
  ret = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  /* Output the formatted string */
  arm_lowputs(buf);

  return ret;
}

/* Early/critical-context logger used by chip debug override. */
int sifli_arch_syslog(int priority, const char *fmt, ...)
{
  va_list ap;
  char buf[256];
  int ret;

  /* Format the message */
  va_start(ap, fmt);
  ret = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  /* Output the formatted string */
  arm_lowputs(buf);

  return ret;
}

#ifdef CONFIG_DEBUG_FEATURES
#  define showprogress(c) arm_lowputc(c)
#else
#  define showprogress(c)
#endif



/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * ROM Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* g_idle_topstack: _sbss is the start of the BSS region as defined by the
 * linker script. _ebss lies at the end of the BSS region. The idle task
 * stack starts at the end of BSS and is of size CONFIG_IDLETHREAD_STACKSIZE.
 * The IDLE thread is the thread that the system boots on and, eventually,
 * becomes the IDLE, do nothing task that runs only when there is nothing
 * else to run.  The heap continues from there until the end of memory.
 * g_idle_topstack is a read-only variable the provides this computed
 * address.
 */

const uintptr_t g_idle_topstack = HEAP_BASE;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* copied from __cmsis_start(void) */
void __cmsis_copy(void)
{
  typedef struct {
    uint32_t const* src;
    uint32_t* dest;
    uint32_t  wlen;
  } __copy_table_t;

  typedef struct {
    uint32_t* dest;
    uint32_t  wlen;
  } __zero_table_t;

  extern const __copy_table_t __copy_table_start__;
  extern const __copy_table_t __copy_table_end__;
  extern const __zero_table_t __zero_table_start__;
  extern const __zero_table_t __zero_table_end__;

  for (__copy_table_t const* pTable = &__copy_table_start__; pTable <
&__copy_table_end__; ++pTable) { for(uint32_t i=0u; i<pTable->wlen; ++i) {
      pTable->dest[i] = pTable->src[i];
    }
  }

  for (__zero_table_t const* pTable = &__zero_table_start__; pTable <
&__zero_table_end__; ++pTable) { for(uint32_t i=0u; i<pTable->wlen; ++i) {
      pTable->dest[i] = 0u;
    }
  }
}

/****************************************************************************
 * Forward declaration — __start branches to __start_c via inline asm.
 ****************************************************************************/

static void __start_c(void);

/****************************************************************************
 * Name: __sifli_start
 *
 * Naked assembly prologue — runs before ANY C code.
 * The ROM bootloader jumps here with its own MSP/MSPLIM/VTOR.
 * We must set up our own MSP and VTOR before the compiler's function
 * prologue touches the stack, otherwise the ROM's tiny stack (40 bytes)
 * overflows or the MPU blocks access → MemManage fault.
 ****************************************************************************/

void __start(void)
{
  /* Naked assembly: set MSP from our vector table, set MSPLIM=0,
   * disable interrupts, set VTOR, then branch to __start_c.
   * No compiler prologue — we haven't set up the stack yet.
   */
  __asm__ __volatile__(
      ".syntax unified\n"
      ".thumb\n"
      "ldr   r0, =_vectors\n"
      "ldr   r1, [r0, #0]\n"    /* r1 = MSP from vector table[0] */
      "msr   msp, r1\n"         /* set MSP to our idle stack */
      "movs  r1, #0\n"
      "msr   msplim, r1\n"      /* clear stack limit */
      "cpsid i\n"               /* disable interrupts */
      "ldr   r1, =0xE000ED08\n" /* SCB->VTOR */
      "str   r0, [r1, #0]\n"    /* set VTOR to our vector table */
      "isb  sy\n"
      "b     __start_c\n"       /* jump to C startup */
      ".ltorg\n"
      : : : "r0", "r1", "memory"
  );

  for (;;);  /* never reached */
}

/* The actual C startup code — called after MSP and VTOR are configured. */
extern void arm_earlyserialinit(void);

static void __start_c(void)
{
  uint32_t *dest;
  const uint32_t *src;

  /* Configure FPU before any floating point operations */

  arm_fpuconfig();
  mpu_config();  

  /* Clear BSS section - critical for proper variable initialization */

  for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; )
    {
      *dest++ = 0;
    }

  /* Copy initialized data from flash to SRAM */

  for (src = (const uint32_t *)_eronly,
       dest = (uint32_t *)_sdata; dest < (uint32_t *)_edata; )
    {
      *dest++ = *src++;
    }

  /* Copy .ramfunc section from flash to SRAM */

  for (src = (const uint32_t *)&_siramfunc,
         dest = (uint32_t *)&_sramfunc; dest < (uint32_t *)&_eramfunc; )
    {
      *dest++ = *src++;
    }

  __cmsis_copy();    

  /* Call HAL_Init() with interrupts disabled.
   * Some HAL functions may trigger hardware events that could
   * generate interrupts, but they won't fire while interrupts are disabled.
   */
  HAL_Init();
  arm_earlyserialinit();

  arm_lowputc('A'); /* data segment init done */

#ifdef CONFIG_ARMV8M_ICACHE
  /* up_enable_icache(); */
#endif

#ifdef CONFIG_ARMV8M_DCACHE
  /* up_enable_dcache(); */
#endif
    arm_lowputc('B'); /* cache enable done */

    arm_lowputc('C'); /* HAL init done */

  /* Disable SysTick that was enabled by HAL_Init().
   * NuttX uses its own timer system (LPTIM for tickless mode).
   * SysTick must be disabled to prevent unexpected interrupts.
   */
#define NVIC_SYSTICK_CTRL_REG   (*((volatile uint32_t *)0xE000E010))
  NVIC_SYSTICK_CTRL_REG = 0;  /* Disable SysTick completely */


  arm_lowputc('D'); /* about to start system */

  /* nx_start() will initialize the interrupt system and enable interrupts.
   * Interrupts remain disabled until the system is fully ready.
   */
  nx_start();
  
  showprogress('X'); /* should never reach here */

  for (; ; );
}
