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

#define MOTOR1_DIR_NODE DT_PATH (motor1_pins, dir)
#define MOTOR1_DIR_LABEL DT_GPIO_LABEL (MOTOR1_DIR_NODE, gpios)
#define MOTOR1_DIR_PIN DT_GPIO_PIN (MOTOR1_DIR_NODE, gpios)
#define MOTOR1_DIR_FLAGS DT_GPIO_FLAGS (MOTOR1_DIR_NODE, gpios)

#define MOTOR1_STEP_NODE DT_PATH (motor1_pins, step)
#define MOTOR1_STEP_LABEL DT_GPIO_LABEL (MOTOR1_STEP_NODE, gpios)
#define MOTOR1_STEP_PIN DT_GPIO_PIN (MOTOR1_STEP_NODE, gpios)
#define MOTOR1_STEP_FLAGS DT_GPIO_FLAGS (MOTOR1_STEP_NODE, gpios)

#define MOTOR1_ENABLE_NODE DT_PATH (motor1_pins, enable)
#define MOTOR1_ENABLE_LABEL DT_GPIO_LABEL (MOTOR1_ENABLE_NODE, gpios)
#define MOTOR1_ENABLE_PIN DT_GPIO_PIN (MOTOR1_ENABLE_NODE, gpios)
#define MOTOR1_ENABLE_FLAGS DT_GPIO_FLAGS (MOTOR1_ENABLE_NODE, gpios)

#define MOTOR1_NSS_NODE DT_PATH (motor1_pins, nss)
#define MOTOR1_NSS_LABEL DT_GPIO_LABEL (MOTOR1_NSS_NODE, gpios)
#define MOTOR1_NSS_PIN DT_GPIO_PIN (MOTOR1_NSS_NODE, gpios)
#define MOTOR1_NSS_FLAGS DT_GPIO_FLAGS (MOTOR1_NSS_NODE, gpios)

/*--------------------------------------------------------------------------*/

#define MOTOR2_DIR_NODE DT_PATH (motor2_pins, dir)
#define MOTOR2_DIR_LABEL DT_GPIO_LABEL (MOTOR2_DIR_NODE, gpios)
#define MOTOR2_DIR_PIN DT_GPIO_PIN (MOTOR2_DIR_NODE, gpios)
#define MOTOR2_DIR_FLAGS DT_GPIO_FLAGS (MOTOR2_DIR_NODE, gpios)

#define MOTOR2_STEP_NODE DT_PATH (motor2_pins, step)
#define MOTOR2_STEP_LABEL DT_GPIO_LABEL (MOTOR2_STEP_NODE, gpios)
#define MOTOR2_STEP_PIN DT_GPIO_PIN (MOTOR2_STEP_NODE, gpios)
#define MOTOR2_STEP_FLAGS DT_GPIO_FLAGS (MOTOR2_STEP_NODE, gpios)

#define MOTOR2_ENABLE_NODE DT_PATH (motor2_pins, enable)
#define MOTOR2_ENABLE_LABEL DT_GPIO_LABEL (MOTOR2_ENABLE_NODE, gpios)
#define MOTOR2_ENABLE_PIN DT_GPIO_PIN (MOTOR2_ENABLE_NODE, gpios)
#define MOTOR2_ENABLE_FLAGS DT_GPIO_FLAGS (MOTOR2_ENABLE_NODE, gpios)

#define MOTOR2_NSS_NODE DT_PATH (motor2_pins, nss)
#define MOTOR2_NSS_LABEL DT_GPIO_LABEL (MOTOR2_NSS_NODE, gpios)
#define MOTOR2_NSS_PIN DT_GPIO_PIN (MOTOR2_NSS_NODE, gpios)
#define MOTOR2_NSS_FLAGS DT_GPIO_FLAGS (MOTOR2_NSS_NODE, gpios)

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

#define MOTOR1_STALL_NODE DT_PATH (edge_switch_pins, motor1_stall)
#define MOTOR1_STALL_LABEL DT_GPIO_LABEL (MOTOR1_STALL_NODE, gpios)
#define MOTOR1_STALL_PIN DT_GPIO_PIN (MOTOR1_STALL_NODE, gpios)
#define MOTOR1_STALL_FLAGS DT_GPIO_FLAGS (MOTOR1_STALL_NODE, gpios)

#define MOTOR2_STALL_NODE DT_PATH (edge_switch_pins, motor2_stall)
#define MOTOR2_STALL_LABEL DT_GPIO_LABEL (MOTOR2_STALL_NODE, gpios)
#define MOTOR2_STALL_PIN DT_GPIO_PIN (MOTOR2_STALL_NODE, gpios)
#define MOTOR2_STALL_FLAGS DT_GPIO_FLAGS (MOTOR2_STALL_NODE, gpios)

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

extern const struct device *dir1;
extern const struct device *step1;
extern const struct device *enable1;
extern const struct device *nss1;

extern const struct device *dir2;
extern const struct device *step2;
extern const struct device *enable2;
extern const struct device *nss2;

extern const struct device *spi;
extern const struct device *pwm;

/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void mcu_peripherals_init ();

#ifdef __cplusplus
}
#endif
