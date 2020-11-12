/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <zephyr.h>

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

// TMC2130 registers
#define WRITE_FLAG (1 << 7) // write flag
#define READ_FLAG (0 << 7)  // read flag
#define REG_GCONF 0x00
#define REG_GSTAT 0x01
#define REG_IHOLD_IRUN 0x10
#define REG_CHOPCONF 0x6C
#define REG_COOLCONF 0x6D
#define REG_DCCTRL 0x6E
#define REG_DRVSTATUS 0x6F

int writeCmd (const struct device *spi, const struct spi_config *spi_cfg, uint8_t cmd, uint32_t value)
{
        // TODO htons.
        const struct spi_buf bufs[] = {{.buf = &cmd, .len = 1}, {.buf = &value, .len = 4}};
        const struct spi_buf_set bufSet = {.buffers = bufs, .count = 2};
        return spi_write (spi, spi_cfg, &bufSet);
}

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

        /*--------------------------------------------------------------------------*/

        const struct device *step = device_get_binding (MOTOR1_STEP_LABEL);

        if (step == NULL) {
                return;
        }

        ret = gpio_pin_configure (step, MOTOR1_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_STEP_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *enable = device_get_binding (MOTOR1_ENABLE_LABEL);

        if (enable == NULL) {
                return;
        }

        ret = gpio_pin_configure (enable, MOTOR1_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_ENABLE_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *nss = device_get_binding (MOTOR1_NSS_LABEL);

        if (nss == NULL) {
                return;
        }

        ret = gpio_pin_configure (nss, MOTOR1_NSS_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_NSS_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        gpio_pin_set (enable, MOTOR1_ENABLE_PIN, false);
        gpio_pin_set (dir, MOTOR1_DIR_PIN, false);

        /*--------------------------------------------------------------------------*/

        struct spi_config spi_cfg = {0};
        const struct device *spi = device_get_binding (DT_LABEL (DT_NODELABEL (spi2)));

        if (!spi) {
                printk ("Could not find SPI driver\n");
                return;
        }

        const struct spi_cs_control spiCs = {.gpio_dev = nss, .gpio_pin = MOTOR1_NSS_PIN, .delay = 0, .gpio_dt_flags = GPIO_ACTIVE_LOW};

        spi_cfg.operation = SPI_WORD_SET (8) | SPI_MODE_CPOL | SPI_MODE_CPHA;
        spi_cfg.frequency = 500000U;
        spi_cfg.cs = &spiCs;

        int err = writeCmd (spi, &spi_cfg, WRITE_FLAG | REG_GCONF, 0x00000001UL);
        err |= writeCmd (spi, &spi_cfg, WRITE_FLAG | REG_IHOLD_IRUN, 0x00001010UL);
        err |= writeCmd (spi, &spi_cfg, WRITE_FLAG | REG_CHOPCONF, 0x08008008UL);

        if (err) {
                printk ("Error writing to SPI! errro code (%d)\n", err);
                return;
        }

        /*--------------------------------------------------------------------------*/

        gpio_pin_set (enable, MOTOR1_ENABLE_PIN, true);

        while (1) {
                gpio_pin_set (step, MOTOR1_STEP_PIN, (int)stepState);
                stepState = !stepState;
                k_msleep (1);
        }
}
