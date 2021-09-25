/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "display.h"
#include "hw_timer.h"
#include "sdCard.h"
#include "stepperDriverSettings.h"
#include "zephyrGrblPeripherals.h"
#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>
#include <sys/reboot.h>
#include <zephyr.h>

LOG_MODULE_REGISTER (main);

extern "C" int grblMain ();
void testStepperMotors ();

/**
 *
 */
void main ()
{
        disp::init ();
        mcuPeripheralsInit ();
        drv::init ();

        // init_nvs ();
        // k_sleep (K_SECONDS (1));
        // sys_reboot (0);

        // while (1) {
        //         k_sleep (K_SECONDS (1));
        // }

        grblMain ();
        // testStepperMotors ();
}

/**
 * Dummy function for motor testing.
 */
void testStepperMotors ()
{
        bool stepState{};
        bool dirState{};

        while (true) {
                gpio_pin_set (dirX, MOTORX_DIR_PIN, dirState);
                gpio_pin_set (dirY, MOTORX_DIR_PIN, dirState);
                dirState = !dirState;

                for (int i = 0; i < 10000; ++i) {
                        gpio_pin_set (stepX, MOTORX_STEP_PIN, (int)stepState);
                        gpio_pin_set (stepY, MOTORY_STEP_PIN, (int)stepState);
                        stepState = !stepState;
                        k_usleep (1000);
                }
        }
}
