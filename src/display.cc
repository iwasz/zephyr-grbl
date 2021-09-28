/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "display.h"
#include "Machine.h"
#include "cfb_font_oldschool.h"
#include "grblState.h"
#include <device.h>
#include <devicetree.h>
#include <display/cfb.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <optional>
#include <zephyr.h>

LOG_MODULE_REGISTER (display);

namespace {
void displayThread (void *, void *, void *);
K_THREAD_DEFINE (disp, 2048, displayThread, NULL, NULL, NULL, 14, 0, 0);
} // namespace

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

        const struct device *display;

        gpio_callback encoderCallback;
        k_timer encoderDebounceTimer;
        // enum class EncoderDebounceState { none = 0x00, a = 0x01, b = 0x02, enter = 0x04 };
        // EncoderDebounceState encoderDebounceState{};
        bool aDebounceState{};
        bool bDebounceState{};
        bool eDebounceState{};

        enum class EncoderDirection { none, left, right };
        EncoderDirection encoderDirection{};
        bool encoderEnter{};

        /**
         * ISR button.
         */
        void encoderCallbackCb (const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
        {
                ARG_UNUSED (cb);

                if (port == encoderA && (pins & BIT (ENCODER_A_PIN))) {
                        aDebounceState = true;
                        k_timer_start (&encoderDebounceTimer, K_USEC (3000), K_NO_WAIT);
                }
                else if (port == encoderB && (pins & BIT (ENCODER_B_PIN))) {
                        bDebounceState = true;
                        k_timer_start (&encoderDebounceTimer, K_USEC (3000), K_NO_WAIT);
                }

                if (port == enter && (pins & BIT (ENCODER_ENTER_PIN))) {
                        eDebounceState = true;
                        k_timer_start (&encoderDebounceTimer, K_MSEC (30), K_NO_WAIT);
                }
        }

        /**
         * Kernel timer callback. Caution! ISR context.
         */
        void encoderDebounceCb (struct k_timer *timer)
        {
                ARG_UNUSED (timer);

                bool a = gpio_pin_get (encoderA, ENCODER_A_PIN);
                bool b = gpio_pin_get (encoderB, ENCODER_B_PIN);

                // A state is consistent
                if (aDebounceState && a) {
                        aDebounceState = false;

                        if (!b) {
                                encoderDirection = EncoderDirection::right;
                        }
                }
                else if (bDebounceState && b) {
                        bDebounceState = false;

                        if (!a) {
                                encoderDirection = EncoderDirection::left;
                        }
                }

                if (eDebounceState && gpio_pin_get (enter, ENCODER_ENTER_PIN)) {
                        eDebounceState = false;
                        encoderEnter = true;
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
        k_timer_init (&encoderDebounceTimer, encoderDebounceCb, nullptr);

        if (ret != 0) {
                LOG_ERR ("Encoder init failed");
        }

        /*--------------------------------------------------------------------------*/

        uint16_t rows;
        uint8_t ppt;
        uint8_t font_width;
        uint8_t font_height;

        display = device_get_binding ("SSD1306");

        if (display == NULL) {
                LOG_ERR ("Device not found\n");
                return;
        }

        if (display_set_pixel_format (display, PIXEL_FORMAT_MONO10) != 0) {
                LOG_ERR ("Failed to set required pixel format\n");
                return;
        }

        if (cfb_framebuffer_init (display)) {
                LOG_ERR ("Framebuffer initialization failed!\n");
                return;
        }

        cfb_framebuffer_clear (display, true);
        display_blanking_off (display);

        rows = cfb_get_display_parameter (display, CFB_DISPLAY_ROWS);
        ppt = cfb_get_display_parameter (display, CFB_DISPLAY_PPT);

        // for (int idx = 0; idx < 42; idx++) {
        //         if (cfb_get_font_size (dev, idx, &font_width, &font_height)) {
        //                 break;
        //         }

        //         // if (font_height == ppt) {
        //         cfb_framebuffer_set_font (dev, idx);
        //         printf ("font width %d, font height %d\n", font_width, font_height);
        //         // }
        // }

        // printf ("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n", cfb_get_display_parameter (dev, CFB_DISPLAY_WIDTH),
        //         cfb_get_display_parameter (dev, CFB_DISPLAY_HEIGH), ppt, rows, cfb_get_display_parameter (dev, CFB_DISPLAY_COLS));

        cfb_framebuffer_set_font (disp::display, 0);
}

} // namespace disp

/**
 * UI related.
 */
namespace {

auto menu = [] (int option) {
        cfb_framebuffer_clear (disp::display, false);
        uint16_t margin = 2;

        cfb_print (disp::display, "1. Y+", margin, 0);
        cfb_print (disp::display, "2. Y-", margin, 8);
        cfb_print (disp::display, "3. X+", margin, 16);
        cfb_print (disp::display, "4. X-", margin, 24);
        cfb_print (disp::display, "5. Ala ma kota", margin, 32);

        cfb_print (disp::display, ">", 2, option * 8);
        cfb_framebuffer_finalize (disp::display);
};

enum class Event { none, left, right, enter };

constexpr auto enter = [] (Event e) { return e == Event::enter; };
constexpr auto left = [] (Event e) { return e == Event::left; };
constexpr auto right = [] (Event e) { return e == Event::right; };

using namespace ls;
auto menuMachine = machine ( // Jog menu
        state ("JOG_YP"_ST, entry ([] { menu (0); }), transition ("JOG_YN"_ST, left), transition ("JOG_XN"_ST, right)),
        state ("JOG_YN"_ST, entry ([] { menu (1); }), transition ("JOG_XP"_ST, left), transition ("JOG_YP"_ST, right)),
        state ("JOG_XP"_ST, entry ([] { menu (2); }), transition ("JOG_XN"_ST, left), transition ("JOG_YN"_ST, right)),
        state ("JOG_XN"_ST, entry ([] { menu (3); }), transition ("JOG_YP"_ST, left), transition ("JOG_XP"_ST, right)));

void displayThread (void *, void *, void *)
{
        using namespace disp;

        // Enter into initial state, and execute its entry state (that displays the menu).
        menuMachine.run (Event::none);

        while (true) {
                if (disp::encoderEnter) {
                        disp::encoderEnter = false;
                        // grbl::jog (grbl::JogDirection::yPositive);
                        // menuMachine.run (Event::enter);
                        printk ("E");
                }

                if (disp::encoderDirection == EncoderDirection::left) {
                        encoderDirection = disp::EncoderDirection::none;
                        // menuMachine.run (Event::left);
                        printk ("L");
                }
                else if (disp::encoderDirection == EncoderDirection::right) {
                        encoderDirection = disp::EncoderDirection::none;
                        // menuMachine.run (Event::right);
                        printk ("R");
                }

                k_sleep (K_MSEC (40));
        }
}
} // namespace
