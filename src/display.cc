/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "display.h"
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <zephyr.h>

LOG_MODULE_REGISTER (display);

namespace disp {

namespace {
#define ENCODER_A_NODE DT_PATH (ui_buttons, encoder_a)
#define ENCODER_A_PIN DT_GPIO_PIN (ENCODER_A_NODE, gpios)
#define ENCODER_A_FLAGS DT_GPIO_FLAGS (ENCODER_A_NODE, gpios)
        const struct device *encoderA;

#define ENCODER_B_NODE DT_PATH (ui_buttons, encoder_b)
#define ENCODER_B_PIN DT_GPIO_PIN (ENCODER_B_NODE, gpios)
#define ENCODER_B_FLAGS DT_GPIO_FLAGS (ENCODER_B_NODE, gpios)
        const struct device *encoderB;

#define ENCODER_ENTER_NODE DT_PATH (ui_buttons, enter)
#define ENCODER_ENTER_PIN DT_GPIO_PIN (ENCODER_ENTER_NODE, gpios)
#define ENCODER_ENTER_FLAGS DT_GPIO_FLAGS (ENCODER_ENTER_NODE, gpios)
        const struct device *enter;

        gpio_callback encoderCallback;

        void encoderCallbackCb (const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
        {
                ARG_UNUSED (cb);

                if (port == encoderA && (pins & BIT (ENCODER_A_PIN))) {
                        printk ("A\r\n");
                }

                if (port == encoderB && (pins & BIT (ENCODER_B_PIN))) {
                        printk ("B\r\n");
                }

                if (port == enter && (pins & BIT (ENCODER_ENTER_PIN))) {
                        printk ("E\r\n");
                }
        }
} // namespace

void init ()
{
        encoderA = device_get_binding (DT_GPIO_LABEL (ENCODER_A_NODE, gpios));

        if (!device_is_ready (encoderA)) {
                LOG_ERR ("encoderA !ready");
        }

        encoderB = device_get_binding (DT_GPIO_LABEL (ENCODER_B_NODE, gpios));

        if (!device_is_ready (encoderB)) {
                LOG_ERR ("encoderB !ready");
        }

        enter = device_get_binding (DT_GPIO_LABEL (ENCODER_ENTER_NODE, gpios));

        if (!device_is_ready (enter)) {
                LOG_ERR ("encoderEnter !ready");
        }

        int ret = gpio_pin_configure (encoderA, ENCODER_A_PIN, GPIO_INPUT | ENCODER_A_FLAGS);
        ret |= gpio_pin_interrupt_configure (encoderA, ENCODER_A_PIN, GPIO_INT_EDGE_TO_ACTIVE);
        ret |= gpio_pin_configure (encoderB, ENCODER_B_PIN, GPIO_INPUT | ENCODER_B_FLAGS);
        ret |= gpio_pin_interrupt_configure (encoderB, ENCODER_B_PIN, GPIO_INT_EDGE_TO_ACTIVE);
        ret |= gpio_pin_configure (enter, ENCODER_ENTER_PIN, GPIO_INPUT | ENCODER_ENTER_FLAGS);
        ret |= gpio_pin_interrupt_configure (enter, ENCODER_ENTER_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        gpio_init_callback (&encoderCallback, encoderCallbackCb, BIT (ENCODER_A_PIN) | BIT (ENCODER_B_PIN) | BIT (ENCODER_ENTER_PIN));
        ret |= gpio_add_callback (encoderA, &encoderCallback);

        if (ret != 0) {
                LOG_ERR ("Encoder init failed");
        }
}

} // namespace disp
