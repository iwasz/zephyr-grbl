/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <zephyr.h>

// /* The devicetree node identifier for the "led0" alias. */
// #define LED0_NODE DT_ALIAS (led0)

// #if DT_NODE_HAS_STATUS(LED0_NODE, okay)
// #define LED0 DT_GPIO_LABEL (LED0_NODE, gpios)
// #define PIN DT_GPIO_PIN (LED0_NODE, gpios)
// #define FLAGS DT_GPIO_FLAGS (LED0_NODE, gpios)
// #else
// /* A build error here means your board isn't set up to blink an LED. */
// #error "Unsupported board: led0 devicetree alias is not defined"
// #define LED0 ""
// #define PIN 0
// #define FLAGS 0
// #endif

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

void main (void)
{
        bool stepState = true;
        int ret;

        const struct device *dir = device_get_binding (MOTOR1_DIR_LABEL);

        if (dir == NULL) {
                return;
        }

        ret = gpio_pin_configure (dir, MOTOR1_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_DIR_FLAGS);

        if (ret < 0) {
                return;
        }

        const struct device *step = device_get_binding (MOTOR1_STEP_LABEL);

        if (step == NULL) {
                return;
        }

        ret = gpio_pin_configure (step, MOTOR1_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_STEP_FLAGS);

        if (ret < 0) {
                return;
        }

        const struct device *enable = device_get_binding (MOTOR1_ENABLE_LABEL);

        if (enable == NULL) {
                return;
        }

        ret = gpio_pin_configure (enable, MOTOR1_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_ENABLE_FLAGS);

        if (ret < 0) {
                return;
        }

        gpio_pin_set (enable, MOTOR1_ENABLE_PIN, true);
        gpio_pin_set (dir, MOTOR1_DIR_PIN, true);

        while (1) {
                gpio_pin_set (step, MOTOR1_STEP_PIN, (int)stepState);
                stepState = !stepState;
                k_msleep (1);
        }
}
