/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "display.h"
#include "exception.h"
#include "hw_timer.h"
#include "sdCard.h"
#include "stepperDriverSettings.h"
#include "zephyrGrblPeripherals.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER (main);

extern "C" int grblMain ();
void testStepperMotors ();

/**
 *
 */
int main ()
{
        try {
                // disp::init ();
                sd::init ();
                mcuPeripheralsInit ();
                drv::init ();

                // init_nvs ();
                // k_sleep (K_SECONDS (1));
                // sys_reboot (0);

                // while (1) {
                //         k_sleep (K_SECONDS (1));
                // }

                // grblMain ();
                testStepperMotors ();
        }
        catch (exc::Exception const &e) {
                printk ("exc::Exception: %s\n", e.msg ().c_str ());
                // exc::pushFromThread (exc::Error{e});
                //  continue;
        }
        catch (std::exception const &e) {
                printk ("std::exception: %s\n", e.what ());
                // exc::pushFromThread (exc::Error{StatusCode::stdException, "Fatal exception"});
                // continue;
        }
        catch (...) {
                printk ("Unknown exception\n");
                // exc::pushFromThread (exc::Error{StatusCode::unknownException, "Unknown exception"});
                // continue;
        }
}

/**
 * Dummy function for motor testing.
 */
void testStepperMotors ()
{
        bool stepState{};
        bool dirState{};

        gpio_pin_set_dt (&enableX, GPIO_OUTPUT_ACTIVE);
        gpio_pin_set_dt (&enableY, GPIO_OUTPUT_ACTIVE);

        while (true) {
                gpio_pin_set_dt (&dirX, dirState);
                gpio_pin_set_dt (&dirY, dirState);
                dirState = !dirState;

                for (int i = 0; i < 10000; ++i) {
                        gpio_pin_set_dt (&stepX, stepState);
                        gpio_pin_set_dt (&stepY, stepState);
                        stepState = !stepState;
                        k_usleep (1000);
                }
        }
}
