#include <TMC2130Stepper.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>
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

// namespace container {

// TMC2130Stepper *aa ()
// {

//         /*--------------------------------------------------------------------------*/

//         // gpio_pin_set (enable, MOTOR1_ENABLE_PIN, true);
//         return &stepper;
// }

// } // namespace container

// int writeCmd (const struct device *spi, const struct spi_config *spiCfg, uint8_t cmd, uint32_t value)
// {
//         value = sys_cpu_to_be32 (value);
//         const struct spi_buf bufs[] = {{.buf = &cmd, .len = 1}, {.buf = &value, .len = 4}};
//         const struct spi_buf_set bufSet = {.buffers = bufs, .count = 2};
//         return spi_write (spi, spiCfg, &bufSet);
// }

/****************************************************************************/

void main (void)
{
        bool stepState = true;

        const struct device *dir = device_get_binding (MOTOR1_DIR_LABEL);

        if (dir == NULL) {
                return;
        }

        int ret = gpio_pin_configure (dir, MOTOR1_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_DIR_FLAGS);

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

        struct spi_config spiCfg = {0};
        const struct device *spi = device_get_binding (DT_LABEL (DT_NODELABEL (spi2)));

        if (!spi) {
                printk ("Could not find SPI driver\n");
                return;
        }

        const struct spi_cs_control spiCs = {.gpio_dev = nss, .delay = 0, .gpio_pin = MOTOR1_NSS_PIN, .gpio_dt_flags = GPIO_ACTIVE_LOW};

        spiCfg.operation = SPI_WORD_SET (8) | SPI_MODE_CPOL | SPI_MODE_CPHA;
        spiCfg.frequency = 500000U;
        spiCfg.cs = &spiCs;

        /*--------------------------------------------------------------------------*/

        TMC2130Stepper driver (spi, &spiCfg);
        driver.rms_current (600);
        driver.stealthChop (1);

        gpio_pin_set (enable, MOTOR1_ENABLE_PIN, true);

        while (1) {
                gpio_pin_set (step, MOTOR1_STEP_PIN, (int)stepState);
                stepState = !stepState;
                k_msleep (1);
        }
}
