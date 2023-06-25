/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "hw_timer.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>

#define PWM_CHANNEL 1
#define PWM_FLAGS HW_TIMER_POLARITY_NORMAL

/****************************************************************************/

extern const struct gpio_dt_spec leftSwitch;
extern const struct gpio_dt_spec rightSwitch;
extern const struct gpio_dt_spec topSwitch;
extern const struct gpio_dt_spec bottomSwitch;
extern const struct gpio_dt_spec motor1Stall;
extern const struct gpio_dt_spec motor2Stall;

extern const struct gpio_dt_spec dirX;
extern const struct gpio_dt_spec stepX;
extern const struct gpio_dt_spec enableX;
extern const struct gpio_dt_spec nssX;

extern const struct gpio_dt_spec dirY;
extern const struct gpio_dt_spec stepY;
extern const struct gpio_dt_spec enableY;
extern const struct gpio_dt_spec nssY;

// const gpio_dt_spec dirZ;
// const gpio_dt_spec stepZ;
// const gpio_dt_spec enableZ;
// const gpio_dt_spec nssZ;

extern const struct device *spi;
extern const struct device *timerCallbackDevice;
extern const struct pwm_dt_spec zAxisPwm;

/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void mcuPeripheralsInit ();

#ifdef __cplusplus
}
#endif
