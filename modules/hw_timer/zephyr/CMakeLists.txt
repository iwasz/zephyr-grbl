# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HW_TIMER_DRIVER)
  # Add hw_timer_driver.h to the set of global include paths.
  zephyr_include_directories(.)

  zephyr_library()
  zephyr_library_sources(
    hw_timer_stm32.c
    )
endif()
