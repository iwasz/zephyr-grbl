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
#include <drivers/uart.h>
#include <logging/log.h>
#include <usb/usb_device.h>
#include <zephyr.h>

LOG_MODULE_REGISTER (mcu_per);

const struct device *leftSwitch{};
const struct device *rightSwitch{};
const struct device *topSwitch{};
const struct device *bottomSwitch{};
const struct device *motor1Stall{};
const struct device *motor2Stall{};

const struct device *dirX{};
const struct device *stepX{};
const struct device *enableX{};
const struct device *nssX{};

const struct device *dirY{};
const struct device *stepY{};
const struct device *enableY{};
const struct device *nssY{};

const struct device *dirZ{};
const struct device *stepZ{};
const struct device *enableZ{};
const struct device *nssZ{};

#ifdef ENABLE_DUAL_AXIS
const struct device *dirXdual{};
const struct device *stepXdual{};
const struct device *enableXdual{};

const struct device *dirYdual{};
const struct device *stepYdual{};
const struct device *enableYdual{};

const struct device *dirZdual{};
const struct device *stepZdual{};
const struct device *enableZdual{};
#endif

const struct device *spi{};
const struct device *pwm{};

/**
 *
 */
void mcuPeripheralsInit ()
{
        int ret{};
        spi = device_get_binding (DT_LABEL (DT_NODELABEL (spi1)));

        if (!spi) {
                printk ("Could not find SPI driver\n");
                return;
        }

        dirX = device_get_binding (MOTORX_DIR_LABEL);

        if (dirX == NULL) {
                return;
        }

        ret = gpio_pin_configure (dirX, MOTORX_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTORX_DIR_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        stepX = device_get_binding (MOTORX_STEP_LABEL);

        if (stepX == NULL) {
                return;
        }

        ret = gpio_pin_configure (stepX, MOTORX_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTORX_STEP_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        enableX = device_get_binding (MOTORX_ENABLE_LABEL);

        if (enableX == NULL) {
                return;
        }

        ret = gpio_pin_configure (enableX, MOTORX_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTORX_ENABLE_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        nssX = device_get_binding (MOTORX_NSS_LABEL);

        if (nssX == NULL) {
                return;
        }

        ret = gpio_pin_configure (nssX, MOTORX_NSS_PIN, GPIO_OUTPUT_ACTIVE | MOTORX_NSS_FLAGS);

        if (ret < 0) {
                return;
        }

        gpio_pin_set (enableX, MOTORX_ENABLE_PIN, false);

        /*--------------------------------------------------------------------------*/

        dirY = device_get_binding (MOTORY_DIR_LABEL);

        if (dirY == NULL) {
                return;
        }

        ret = gpio_pin_configure (dirY, MOTORY_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTORY_DIR_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        stepY = device_get_binding (MOTORY_STEP_LABEL);

        if (stepY == NULL) {
                return;
        }

        ret = gpio_pin_configure (stepY, MOTORY_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTORY_STEP_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        enableY = device_get_binding (MOTORY_ENABLE_LABEL);

        if (enableY == NULL) {
                return;
        }

        ret = gpio_pin_configure (enableY, MOTORY_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTORY_ENABLE_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        nssY = device_get_binding (MOTORY_NSS_LABEL);

        if (nssY == NULL) {
                return;
        }

        ret = gpio_pin_configure (nssY, MOTORY_NSS_PIN, GPIO_OUTPUT_ACTIVE | MOTORY_NSS_FLAGS);

        if (ret < 0) {
                return;
        }

        gpio_pin_set (enableY, MOTORY_ENABLE_PIN, false);

        /*--------------------------------------------------------------------------*/

        dirZ = device_get_binding (MOTORZ_DIR_LABEL);

        if (dirZ == NULL || gpio_pin_configure (dirZ, MOTORZ_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTORZ_DIR_FLAGS) < 0) {
                LOG_ERR ("dirZ pin configuration error");
                return;
        }

        stepZ = device_get_binding (MOTORZ_STEP_LABEL);

        if (stepZ == NULL || gpio_pin_configure (stepZ, MOTORZ_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTORZ_STEP_FLAGS) < 0) {
                LOG_ERR ("stepZ pin configuration error");
                return;
        }

        enableZ = device_get_binding (MOTORZ_ENABLE_LABEL);

        if (enableZ == NULL || gpio_pin_configure (enableZ, MOTORZ_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTORZ_ENABLE_FLAGS) < 0) {
                LOG_ERR ("enableZ pin configuration error");
                return;
        }

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

        motor1Stall = device_get_binding (MOTORX_STALL_LABEL);

        if (motor1Stall == NULL) {
                return;
        }

        ret = gpio_pin_configure (motor1Stall, MOTORX_STALL_PIN, GPIO_INPUT | MOTORX_STALL_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (motor1Stall, MOTORX_STALL_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        /*--------------------------------------------------------------------------*/

        motor2Stall = device_get_binding (MOTORY_STALL_LABEL);

        if (motor2Stall == NULL) {
                return;
        }

        ret = gpio_pin_configure (motor2Stall, MOTORY_STALL_PIN, GPIO_INPUT | MOTORY_STALL_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (motor2Stall, MOTORY_STALL_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        /*--------------------------------------------------------------------------*/

        if (!DT_NODE_EXISTS (DT_NODELABEL (grbl_callback))) {
                // Nie istnieje
        }

        if (!DT_NODE_HAS_PROP (DT_NODELABEL (grbl_callback), label)) {
                // Nie ma labela
        }

        pwm = device_get_binding (DT_PROP (DT_NODELABEL (grbl_callback), label));

        if (pwm == nullptr) {
                LOG_ERR ("No grbl callback has been found. Check your device tree.");
                return;
        }

        /*--------------------------------------------------------------------------*/

        // #define PWM_LED0_NODE DT_ALIAS (grblpwm)
        // #define PWM_LABEL DT_PWMS_LABEL (PWM_LED0_NODE)
        //  int a = DT_PWMS_CHANNEL (DT_NODELABEL (grbl_callback)));
        // DT_PWMS_FLAGS (DT_NODELABEL (grbl_callback)))

        // pwm = device_get_binding (PWM_LABEL);

        // if (!pwm) {
        //         printk ("Error: didn't find %s device\n", PWM_LABEL);
        //         return;
        // }

        /*--------------------------------------------------------------------------*/

        /*
         * device_get_binding function gets device by name, while DEVICE_DT_GET_ONE
         * gets device by its 'compatible' property.
         *
         * Here I'm only initializing the USB and CDC_ACM, while GRBL refers to the UART
         * by DTS alias (grbluart). This way GRBL is independent of concrete UART implementation.
         */
        const struct device *dev = DEVICE_DT_GET_ONE (zephyr_cdc_acm_uart);

        if (dev == nullptr) {
                LOG_ERR ("No zephyr_cdc_acm_uart compatible device");
        }

        if (!device_is_ready (dev)) {
                LOG_ERR ("CDC ACM device not ready");
                return;
        }

        ret = usb_enable (NULL);

        if (ret != 0) {
                LOG_ERR ("Failed to enable USB");
                return;
        }

        // ring_buf_init (&ringbuf, sizeof (ring_buffer), ring_buffer);

        LOG_INF ("Wait for DTR");
        uint32_t dtr = 0U;

        // TODO This blocks until somebody connects. Why is that? What is it for?
        while (true) {
                uart_line_ctrl_get (dev, UART_LINE_CTRL_DTR, &dtr);

                if (dtr) {
                        break;
                }
                else {
                        /* Give CPU resources to low priority threads. */
                        k_sleep (K_MSEC (100));
                }
        }

        // TODO I can't see this logs (even LOG_ERR is not printed out on the console).
        LOG_INF ("DTR set");

        /* They are optional, we use them to test the interrupt endpoint */
        ret = uart_line_ctrl_set (dev, UART_LINE_CTRL_DCD, 1);

        if (ret) {
                LOG_WRN ("Failed to set DCD, ret code %d", ret);
        }

        ret = uart_line_ctrl_set (dev, UART_LINE_CTRL_DSR, 1);

        if (ret) {
                LOG_WRN ("Failed to set DSR, ret code %d", ret);
        }

        /* Wait 1 sec for the host to do all settings */
        k_busy_wait (1000000);

        uint32_t baudrate{};
        ret = uart_line_ctrl_get (dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);

        if (ret) {
                LOG_WRN ("Failed to get baudrate, ret code %d", ret);
        }
        else {
                LOG_INF ("Baudrate detected: %d", baudrate);
        }
}
