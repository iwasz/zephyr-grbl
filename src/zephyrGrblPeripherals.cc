/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "zephyrGrblPeripherals.h"
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER (mcu_per);

const struct gpio_dt_spec leftSwitch = GPIO_DT_SPEC_GET (DT_PATH (motor1_pins, dir), gpios);
const struct gpio_dt_spec rightSwitch = GPIO_DT_SPEC_GET (DT_PATH (motor1_pins, dir), gpios);
const struct gpio_dt_spec topSwitch = GPIO_DT_SPEC_GET (DT_PATH (motor1_pins, dir), gpios);
const struct gpio_dt_spec bottomSwitch = GPIO_DT_SPEC_GET (DT_PATH (motor1_pins, dir), gpios);
const struct gpio_dt_spec motor1Stall = GPIO_DT_SPEC_GET (DT_PATH (motor1_pins, dir), gpios);
const struct gpio_dt_spec motor2Stall = GPIO_DT_SPEC_GET (DT_PATH (motor1_pins, dir), gpios);

const struct gpio_dt_spec dirX = GPIO_DT_SPEC_GET (DT_PATH (motor1_pins, dir), gpios);
const struct gpio_dt_spec stepX = GPIO_DT_SPEC_GET (DT_PATH (motor1_pins, step), gpios);
const struct gpio_dt_spec enableX = GPIO_DT_SPEC_GET (DT_PATH (motor1_pins, enable), gpios);
const struct gpio_dt_spec nssX = GPIO_DT_SPEC_GET (DT_PATH (motor1_pins, nss), gpios);

const struct gpio_dt_spec dirY = GPIO_DT_SPEC_GET (DT_PATH (motor2_pins, dir), gpios);
const struct gpio_dt_spec stepY = GPIO_DT_SPEC_GET (DT_PATH (motor2_pins, step), gpios);
const struct gpio_dt_spec enableY = GPIO_DT_SPEC_GET (DT_PATH (motor2_pins, enable), gpios);
const struct gpio_dt_spec nssY = GPIO_DT_SPEC_GET (DT_PATH (motor2_pins, nss), gpios);

const device *spi{};
const device *timerCallbackDevice{};
const struct pwm_dt_spec zAxisPwm = PWM_DT_SPEC_GET (DT_NODELABEL (servopwm));

/**
 *
 */
void mcuPeripheralsInit ()
{
        int ret{};
        spi = DEVICE_DT_GET (DT_ALIAS (motorspi));

        if (!spi) {
                printk ("Could not find SPI driver\n");
                return;
        }

        /*--------------------------------------------------------------------------*/

        if (!gpio_is_ready_dt (&dirX)) {
                printk ("Error\r\n");
                return;
        }

        if (int const ret = gpio_pin_configure_dt (&dirX, GPIO_OUTPUT_ACTIVE); ret != 0) {
                printk ("Error\r\n");
                return;
        }

        if (!gpio_is_ready_dt (&stepX)) {
                printk ("Error\r\n");
                return;
        }

        if (int const ret = gpio_pin_configure_dt (&stepX, GPIO_OUTPUT_ACTIVE); ret != 0) {
                printk ("Error\r\n");
                return;
        }

        if (!gpio_is_ready_dt (&enableX)) {
                printk ("Error\r\n");
                return;
        }

        if (int const ret = gpio_pin_configure_dt (&enableX, GPIO_OUTPUT_INACTIVE); ret != 0) {
                printk ("Error\r\n");
                return;
        }

        if (!gpio_is_ready_dt (&nssX)) {
                printk ("Error\r\n");
                return;
        }

        if (int const ret = gpio_pin_configure_dt (&nssX, GPIO_OUTPUT_INACTIVE); ret != 0) {
                printk ("Error\r\n");
                return;
        }

        /*--------------------------------------------------------------------------*/

        if (!gpio_is_ready_dt (&dirY)) {
                printk ("Error\r\n");
                return;
        }

        if (int const ret = gpio_pin_configure_dt (&dirY, GPIO_OUTPUT_ACTIVE); ret != 0) {
                printk ("Error\r\n");
                return;
        }

        if (!gpio_is_ready_dt (&stepY)) {
                printk ("Error\r\n");
                return;
        }

        if (int const ret = gpio_pin_configure_dt (&stepY, GPIO_OUTPUT_ACTIVE); ret != 0) {
                printk ("Error\r\n");
                return;
        }

        if (!gpio_is_ready_dt (&enableY)) {
                printk ("Error\r\n");
                return;
        }

        if (int const ret = gpio_pin_configure_dt (&enableY, GPIO_OUTPUT_INACTIVE); ret != 0) {
                printk ("Error\r\n");
                return;
        }

        if (!gpio_is_ready_dt (&nssY)) {
                printk ("Error\r\n");
                return;
        }

        if (int const ret = gpio_pin_configure_dt (&nssY, GPIO_OUTPUT_INACTIVE); ret != 0) {
                printk ("Error\r\n");
                return;
        }

        /*--------------------------------------------------------------------------*/

        if (!DT_NODE_EXISTS (DT_NODELABEL (grbl_callback))) {
                // Nie istnieje
        }

        if (!DT_NODE_HAS_PROP (DT_NODELABEL (grbl_callback), label)) {
                // Nie ma labela
        }

        timerCallbackDevice = device_get_binding (DT_PROP (DT_NODELABEL (grbl_callback), label));

        if (timerCallbackDevice == nullptr) {
                LOG_ERR ("No grbl callback has been found. Check your device tree.");
                return;
        }

        /*--------------------------------------------------------------------------*/

        if (!device_is_ready (zAxisPwm.dev)) {
                LOG_ERR ("No axis Z PWM device.");
                return;
        }

        // TODO reltive to frequency from DT
        // TODO or use servo devicetree definition, but it doesn't and I dont know why
        ret = pwm_set_pulse_dt (&zAxisPwm, PWM_MSEC (15));

        if (ret) {
                LOG_ERR ("Error %d: failed to set pulse width\n", ret);
                return;
        }

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

        // TODO This blocks until somebody connects. Why is it that way? What is it for?
        while (true) {
                uart_line_ctrl_get (dev, UART_LINE_CTRL_DTR, &dtr);

                if (dtr) {
                        break;
                }

                /* Give CPU resources to low priority threads. */
                k_sleep (K_MSEC (100));
        }

        LOG_INF ("DTR set");

        // /* They are optional, we use them to test the interrupt endpoint */
        // ret = uart_line_ctrl_set (dev, UART_LINE_CTRL_DCD, 1);

        // if (ret) {
        //         LOG_WRN ("Failed to set DCD, ret code %d", ret);
        // }

        // ret = uart_line_ctrl_set (dev, UART_LINE_CTRL_DSR, 1);

        // if (ret) {
        //         LOG_WRN ("Failed to set DSR, ret code %d", ret);
        // }

        /* Wait 1 sec for the host to do all settings */
        // k_busy_wait (1000000);

        uint32_t baudrate{};
        ret = uart_line_ctrl_get (dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);

        if (ret) {
                LOG_WRN ("Failed to get baudrate, ret code %d", ret);
        }
        else {
                LOG_INF ("Baudrate detected: %d", baudrate);
        }
}
