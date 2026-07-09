# Contest 2026 team 162 hello app
# Copyright (C) 2026 Yang Hongbo <yang.hongbo@iotpi.xyz>
# SPDX-License-Identifier: Apache-2.0
if(CONFIG_LVX_USE_DEMO_CONTEST2026_000_HELLO_APP)
  nuttx_add_application(
    NAME hello_app
    SRCS hello_app_main.c
    STACKSIZE 2048)
endif()
