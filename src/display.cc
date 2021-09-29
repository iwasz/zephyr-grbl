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
#include <array>
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
        const struct device *encoderEnter;

        const struct device *display;
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

        encoderEnter = device_get_binding (DT_GPIO_LABEL (ENCODER_ENTER_NODE, gpios));

        if (!device_is_ready (encoderEnter)) {
                LOG_ERR ("encoderEnter !ready");
        }

        int ret = gpio_pin_configure (encoderA, ENCODER_A_PIN, GPIO_INPUT | ENCODER_A_FLAGS);
        ret |= gpio_pin_configure (encoderB, ENCODER_B_PIN, GPIO_INPUT | ENCODER_B_FLAGS);
        ret |= gpio_pin_configure (encoderEnter, ENCODER_ENTER_PIN, GPIO_INPUT | ENCODER_ENTER_FLAGS);

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
auto menuMachine = machine (                          // Jog menu
        state ("JOG_YP"_ST, entry ([] { menu (0); }), // Jog along Y axis in positive direction.
               transition ("JOG_YN"_ST, left),        // Prev menu item
               transition ("JOG_XN"_ST, right),       // Next menu item
               transition ("JOG_YP"_ST, enter, [] (auto) { grbl::jog (grbl::JogDirection::yPositive); })),

        state ("JOG_YN"_ST, entry ([] { menu (1); }), //
               transition ("JOG_XP"_ST, left),        //
               transition ("JOG_YP"_ST, right),       //
               transition ("JOG_YN"_ST, enter, [] (auto) { grbl::jog (grbl::JogDirection::yNegative); })),

        state ("JOG_XP"_ST, entry ([] { menu (2); }), //
               transition ("JOG_XN"_ST, left),        //
               transition ("JOG_YN"_ST, right),       //
               transition ("JOG_XP"_ST, enter, [] (auto) { grbl::jog (grbl::JogDirection::xPositive); })),

        state ("JOG_XN"_ST, entry ([] { menu (3); }), //
               transition ("JOG_YP"_ST, left),        //
               transition ("JOG_XP"_ST, right),       //
               transition ("JOG_XN"_ST, enter, [] (auto) { grbl::jog (grbl::JogDirection::yPositive); })));

void displayThread (void *, void *, void *)
{
        using namespace disp;

        // Enter into initial state, and execute its entry state (that displays the menu).
        menuMachine.run (Event::none);
        bool prevE{};
        uint8_t state = 0x01;

        uint8_t output{};

        while (true) {
                constexpr std::array<std::array<uint8_t, 4>, 4> ENC = {{
                        {0x00, 0x01, 0x02, 0x01},
                        {0x10, 0x01, 0x01, 0x23},
                        {0x20, 0x01, 0x02, 0x13},
                        {0x01, 0x01, 0x02, 0x03},
                }};

                uint8_t event = gpio_pin_get (encoderB, ENCODER_B_PIN) << 1 | gpio_pin_get (encoderA, ENCODER_A_PIN);
                auto x = ENC[state][event];
                state = x & 0x03;
                output = x & 0x30;

                if (bool e = gpio_pin_get (encoderEnter, ENCODER_ENTER_PIN); e != prevE) {
                        prevE = e;

                        if (e) {
                                menuMachine.run (Event::enter);
                        }
                }

                if (output == 0x20) {
                        menuMachine.run (Event::left);
                }
                else if (output == 0x10) {
                        menuMachine.run (Event::right);
                }

                k_sleep (K_MSEC (4));
        }
}
} // namespace
