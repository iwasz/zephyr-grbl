/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <device.h>

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

#define MOTORX_STALL_NODE DT_PATH (edge_switch_pins, motor1_stall)
#define MOTORX_STALL_LABEL DT_GPIO_LABEL (MOTORX_STALL_NODE, gpios)
#define MOTORX_STALL_PIN DT_GPIO_PIN (MOTORX_STALL_NODE, gpios)
#define MOTORX_STALL_FLAGS DT_GPIO_FLAGS (MOTORX_STALL_NODE, gpios)

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

#define MOTORY_STALL_NODE DT_PATH (edge_switch_pins, motor2_stall)
#define MOTORY_STALL_LABEL DT_GPIO_LABEL (MOTORY_STALL_NODE, gpios)
#define MOTORY_STALL_PIN DT_GPIO_PIN (MOTORY_STALL_NODE, gpios)
#define MOTORY_STALL_FLAGS DT_GPIO_FLAGS (MOTORY_STALL_NODE, gpios)

/*--------------------------------------------------------------------------*/

#define MOTORZ_DIR_NODE DT_PATH (motorz_pins, dir)
#define MOTORZ_DIR_LABEL DT_GPIO_LABEL (MOTORZ_DIR_NODE, gpios)
#define MOTORZ_DIR_PIN DT_GPIO_PIN (MOTORZ_DIR_NODE, gpios)
#define MOTORZ_DIR_FLAGS DT_GPIO_FLAGS (MOTORZ_DIR_NODE, gpios)

#define MOTORZ_STEP_NODE DT_PATH (motorz_pins, step)
#define MOTORZ_STEP_LABEL DT_GPIO_LABEL (MOTORZ_STEP_NODE, gpios)
#define MOTORZ_STEP_PIN DT_GPIO_PIN (MOTORZ_STEP_NODE, gpios)
#define MOTORZ_STEP_FLAGS DT_GPIO_FLAGS (MOTORZ_STEP_NODE, gpios)

#define MOTORZ_ENABLE_NODE DT_PATH (motorz_pins, enable)
#define MOTORZ_ENABLE_LABEL DT_GPIO_LABEL (MOTORZ_ENABLE_NODE, gpios)
#define MOTORZ_ENABLE_PIN DT_GPIO_PIN (MOTORZ_ENABLE_NODE, gpios)
#define MOTORZ_ENABLE_FLAGS DT_GPIO_FLAGS (MOTORZ_ENABLE_NODE, gpios)

/*--------------------------------------------------------------------------*/

#define SWITCH_LEFT_NODE DT_PATH (edge_switch_pins, left)
#define SWITCH_LEFT_LABEL DT_GPIO_LABEL (SWITCH_LEFT_NODE, gpios)
#define SWITCH_LEFT_PIN DT_GPIO_PIN (SWITCH_LEFT_NODE, gpios)
#define SWITCH_LEFT_FLAGS DT_GPIO_FLAGS (SWITCH_LEFT_NODE, gpios)

#define SWITCH_RIGHT_NODE DT_PATH (edge_switch_pins, right)
#define SWITCH_RIGHT_LABEL DT_GPIO_LABEL (SWITCH_RIGHT_NODE, gpios)
#define SWITCH_RIGHT_PIN DT_GPIO_PIN (SWITCH_RIGHT_NODE, gpios)
#define SWITCH_RIGHT_FLAGS DT_GPIO_FLAGS (SWITCH_RIGHT_NODE, gpios)

#define SWITCH_TOP_NODE DT_PATH (edge_switch_pins, top)
#define SWITCH_TOP_LABEL DT_GPIO_LABEL (SWITCH_TOP_NODE, gpios)
#define SWITCH_TOP_PIN DT_GPIO_PIN (SWITCH_TOP_NODE, gpios)
#define SWITCH_TOP_FLAGS DT_GPIO_FLAGS (SWITCH_TOP_NODE, gpios)

#define SWITCH_BOTTOM_NODE DT_PATH (edge_switch_pins, bottom)
#define SWITCH_BOTTOM_LABEL DT_GPIO_LABEL (SWITCH_BOTTOM_NODE, gpios)
#define SWITCH_BOTTOM_PIN DT_GPIO_PIN (SWITCH_BOTTOM_NODE, gpios)
#define SWITCH_BOTTOM_FLAGS DT_GPIO_FLAGS (SWITCH_BOTTOM_NODE, gpios)

/*--------------------------------------------------------------------------*/

#define PWM_LED0_NODE DT_ALIAS (pwm_led0)
#define PWM_LABEL DT_PWMS_LABEL (PWM_LED0_NODE)
#define PWM_CHANNEL DT_PWMS_CHANNEL (PWM_LED0_NODE)
#define PWM_FLAGS DT_PWMS_FLAGS (PWM_LED0_NODE)

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
extern const struct device *pwm;

/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void mcu_peripherals_init ();
void pwm_set_prescaler (); // TODO

#ifdef __cplusplus
}
#endif
