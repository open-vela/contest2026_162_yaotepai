/****************************************************************************
 * board/sf32lb58-lcd_a128r32n1_qspi/src/sf32lb58_lcd_a128r32n1_qspi.h
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

#ifndef __BOARD_SF32LB58_LCD_A128R32N1_QSPI_H
#define __BOARD_SF32LB58_LCD_A128R32N1_QSPI_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Number of GPIO pins */
#define BOARD_NGPIOIN     1
#define BOARD_NGPIOOUT    1
#define BOARD_NGPIOINT    1

#define GPIO_IN1          (GET_PIN_2(hwp_gpio1, 34))
#define GPIO_OUT1         (GET_PIN_2(hwp_gpio1, 26))

#define GPIO_INT1         (GET_PIN_2(hwp_gpio1, 34))



int sf32lb58_lcd_bringup(void);

#ifdef CONFIG_INPUT_BUTTONS
int sf32lb58_button_initialize(const char *devname);
#endif


#ifdef CONFIG_DEV_GPIO
int sifli_gpio_initialize(void);
#endif

#endif /* __BOARD_SF32LB58_LCD_A128R32N1_QSPI_H */

