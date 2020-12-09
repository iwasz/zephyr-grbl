/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "mcu-peripherals.h"
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include <drivers/spi.h>
#include <zephyr.h>

const struct device *leftSwitch{};
const struct device *rightSwitch{};
const struct device *topSwitch{};
const struct device *bottomSwitch{};
const struct device *motor1Stall{};
const struct device *motor2Stall{};

const struct device *dir1{};
const struct device *step1{};
const struct device *enable1{};
const struct device *nss1{};

const struct device *dir2{};
const struct device *step2{};
const struct device *enable2{};
const struct device *nss2{};

const struct device *spi{};
const struct device *pwm{};

void mcu_peripherals_init ()
{
        int ret{};
        spi = device_get_binding (DT_LABEL (DT_NODELABEL (spi1)));

        if (!spi) {
                printk ("Could not find SPI driver\n");
                return;
        }

        dir1 = device_get_binding (MOTOR1_DIR_LABEL);

        if (dir1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (dir1, MOTOR1_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_DIR_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        step1 = device_get_binding (MOTOR1_STEP_LABEL);

        if (step1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (step1, MOTOR1_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_STEP_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        enable1 = device_get_binding (MOTOR1_ENABLE_LABEL);

        if (enable1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (enable1, MOTOR1_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_ENABLE_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        nss1 = device_get_binding (MOTOR1_NSS_LABEL);

        if (nss1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (nss1, MOTOR1_NSS_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_NSS_FLAGS);

        if (ret < 0) {
                return;
        }

        gpio_pin_set (enable1, MOTOR1_ENABLE_PIN, false);

        /*--------------------------------------------------------------------------*/

        dir2 = device_get_binding (MOTOR2_DIR_LABEL);

        if (dir2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (dir2, MOTOR2_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_DIR_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        step2 = device_get_binding (MOTOR2_STEP_LABEL);

        if (step2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (step2, MOTOR2_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_STEP_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        enable2 = device_get_binding (MOTOR2_ENABLE_LABEL);

        if (enable2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (enable2, MOTOR2_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_ENABLE_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        nss2 = device_get_binding (MOTOR2_NSS_LABEL);

        if (nss2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (nss2, MOTOR2_NSS_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_NSS_FLAGS);

        if (ret < 0) {
                return;
        }

        gpio_pin_set (enable2, MOTOR2_ENABLE_PIN, false);

        /*--------------------------------------------------------------------------*/

        leftSwitch = device_get_binding (SWITCH_LEFT_LABEL);

        if (leftSwitch == NULL) {
                return;
        }

        ret = gpio_pin_configure (leftSwitch, SWITCH_LEFT_PIN, GPIO_INPUT | SWITCH_LEFT_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (leftSwitch, SWITCH_LEFT_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        /*--------------------------------------------------------------------------*/

        rightSwitch = device_get_binding (SWITCH_RIGHT_LABEL);

        if (rightSwitch == NULL) {
                return;
        }

        ret = gpio_pin_configure (rightSwitch, SWITCH_RIGHT_PIN, GPIO_INPUT | SWITCH_RIGHT_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (rightSwitch, SWITCH_RIGHT_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        /*--------------------------------------------------------------------------*/

        topSwitch = device_get_binding (SWITCH_TOP_LABEL);

        if (topSwitch == NULL) {
                return;
        }

        ret = gpio_pin_configure (topSwitch, SWITCH_TOP_PIN, GPIO_INPUT | SWITCH_TOP_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (topSwitch, SWITCH_TOP_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        /*--------------------------------------------------------------------------*/

        bottomSwitch = device_get_binding (SWITCH_BOTTOM_LABEL);

        if (bottomSwitch == NULL) {
                return;
        }

        ret = gpio_pin_configure (bottomSwitch, SWITCH_BOTTOM_PIN, GPIO_INPUT | SWITCH_BOTTOM_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (bottomSwitch, SWITCH_BOTTOM_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        /*--------------------------------------------------------------------------*/

        motor1Stall = device_get_binding (MOTOR1_STALL_LABEL);

        if (motor1Stall == NULL) {
                return;
        }

        ret = gpio_pin_configure (motor1Stall, MOTOR1_STALL_PIN, GPIO_INPUT | MOTOR1_STALL_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (motor1Stall, MOTOR1_STALL_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        /*--------------------------------------------------------------------------*/

        motor2Stall = device_get_binding (MOTOR2_STALL_LABEL);

        if (motor2Stall == NULL) {
                return;
        }

        ret = gpio_pin_configure (motor2Stall, MOTOR2_STALL_PIN, GPIO_INPUT | MOTOR2_STALL_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (motor2Stall, MOTOR2_STALL_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        /*--------------------------------------------------------------------------*/

        pwm = device_get_binding (PWM_LABEL);

        if (!pwm) {
                printk ("Error: didn't find %s device\n", PWM_LABEL);
                return;
        }
}