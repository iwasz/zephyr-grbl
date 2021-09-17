/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "hw_timer.h"
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include <drivers/spi.h>
#include <drivers/uart.h>
#include <logging/log.h>
#include <usb/usb_device.h>
#include <zephyr.h>

/*--------------------------------------------------------------------------*/

// TODO change names from MOTOR1 to MOTORX etc
// TODO this sort of defines were originally defined in grbl's cpu_map.h. There's still some left there.
#define MOTORX_DIR_NODE DT_PATH (motor1_pins, dir)
#define MOTORX_DIR_LABEL DT_GPIO_LABEL (MOTORX_DIR_NODE, gpios)
#define MOTORX_DIR_PIN DT_GPIO_PIN (MOTORX_DIR_NODE, gpios)
#define MOTORX_DIR_FLAGS DT_GPIO_FLAGS (MOTORX_DIR_NODE, gpios)

#define MOTORX_STEP_NODE DT_PATH (motor1_pins, step)
#define MOTORX_STEP_LABEL DT_GPIO_LABEL (MOTORX_STEP_NODE, gpios)
#define MOTORX_STEP_PIN DT_GPIO_PIN (MOTORX_STEP_NODE, gpios)
#define MOTORX_STEP_FLAGS DT_GPIO_FLAGS (MOTORX_STEP_NODE, gpios)

#define MOTORX_ENABLE_NODE DT_PATH (motor1_pins, enable)
#define MOTORX_ENABLE_LABEL DT_GPIO_LABEL (MOTORX_ENABLE_NODE, gpios)
#define MOTORX_ENABLE_PIN DT_GPIO_PIN (MOTORX_ENABLE_NODE, gpios)
#define MOTORX_ENABLE_FLAGS DT_GPIO_FLAGS (MOTORX_ENABLE_NODE, gpios)

#define MOTORX_NSS_NODE DT_PATH (motor1_pins, nss)
#define MOTORX_NSS_LABEL DT_GPIO_LABEL (MOTORX_NSS_NODE, gpios)
#define MOTORX_NSS_PIN DT_GPIO_PIN (MOTORX_NSS_NODE, gpios)
#define MOTORX_NSS_FLAGS DT_GPIO_FLAGS (MOTORX_NSS_NODE, gpios)

/*--------------------------------------------------------------------------*/

#define MOTORY_DIR_NODE DT_PATH (motor2_pins, dir)
#define MOTORY_DIR_LABEL DT_GPIO_LABEL (MOTORY_DIR_NODE, gpios)
#define MOTORY_DIR_PIN DT_GPIO_PIN (MOTORY_DIR_NODE, gpios)
#define MOTORY_DIR_FLAGS DT_GPIO_FLAGS (MOTORY_DIR_NODE, gpios)

#define MOTORY_STEP_NODE DT_PATH (motor2_pins, step)
#define MOTORY_STEP_LABEL DT_GPIO_LABEL (MOTORY_STEP_NODE, gpios)
#define MOTORY_STEP_PIN DT_GPIO_PIN (MOTORY_STEP_NODE, gpios)
#define MOTORY_STEP_FLAGS DT_GPIO_FLAGS (MOTORY_STEP_NODE, gpios)

#define MOTORY_ENABLE_NODE DT_PATH (motor2_pins, enable)
#define MOTORY_ENABLE_LABEL DT_GPIO_LABEL (MOTORY_ENABLE_NODE, gpios)
#define MOTORY_ENABLE_PIN DT_GPIO_PIN (MOTORY_ENABLE_NODE, gpios)
#define MOTORY_ENABLE_FLAGS DT_GPIO_FLAGS (MOTORY_ENABLE_NODE, gpios)

#define MOTORY_NSS_NODE DT_PATH (motor2_pins, nss)
#define MOTORY_NSS_LABEL DT_GPIO_LABEL (MOTORY_NSS_NODE, gpios)
#define MOTORY_NSS_PIN DT_GPIO_PIN (MOTORY_NSS_NODE, gpios)
#define MOTORY_NSS_FLAGS DT_GPIO_FLAGS (MOTORY_NSS_NODE, gpios)

/*--------------------------------------------------------------------------*/

// #define PWM_LED0_NODE DT_ALIAS (pwm_led0)
// #define PWM_LED0_NODE DT_ALIAS (grblpwm)
// #define PWM_LABEL DT_PWMS_LABEL (PWM_LED0_NODE) // PWM_LABEL is designed so it gets the phandle of a device from the "pwms" property.
// #define PWM_CHANNEL DT_PWMS_CHANNEL (PWM_LED0_NODE)
// #define PWM_FLAGS DT_PWMS_FLAGS (PWM_LED0_NODE)

#define PWM_CHANNEL 1
#define PWM_FLAGS HW_TIMER_POLARITY_NORMAL

/****************************************************************************/

extern const struct device *leftSwitch;
extern const struct device *rightSwitch;
extern const struct device *topSwitch;
extern const struct device *bottomSwitch;
extern const struct device *motor1Stall;
extern const struct device *motor2Stall;

extern const struct device *dirX;
extern const struct device *stepX;
extern const struct device *enableX;
extern const struct device *nssX;

extern const struct device *dirY;
extern const struct device *stepY;
extern const struct device *enableY;
extern const struct device *nssY;

extern const struct device *dirZ;
extern const struct device *stepZ;
extern const struct device *enableZ;
extern const struct device *nssZ;

extern const struct device *spi;
extern const struct device *timerCallbackDevice;
extern const struct device *zAxisPwm;

/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void mcuPeripheralsInit ();

#ifdef __cplusplus
}
#endif
