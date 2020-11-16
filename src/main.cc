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

/*--------------------------------------------------------------------------*/

#define MOTOR2_DIR_NODE DT_PATH (motor2_pins, dir)
#define MOTOR2_DIR_LABEL DT_GPIO_LABEL (MOTOR2_DIR_NODE, gpios)
#define MOTOR2_DIR_PIN DT_GPIO_PIN (MOTOR2_DIR_NODE, gpios)
#define MOTOR2_DIR_FLAGS DT_GPIO_FLAGS (MOTOR2_DIR_NODE, gpios)

#define MOTOR2_STEP_NODE DT_PATH (motor2_pins, step)
#define MOTOR2_STEP_LABEL DT_GPIO_LABEL (MOTOR2_STEP_NODE, gpios)
#define MOTOR2_STEP_PIN DT_GPIO_PIN (MOTOR2_STEP_NODE, gpios)
#define MOTOR2_STEP_FLAGS DT_GPIO_FLAGS (MOTOR2_STEP_NODE, gpios)

#define MOTOR2_ENABLE_NODE DT_PATH (motor2_pins, enable)
#define MOTOR2_ENABLE_LABEL DT_GPIO_LABEL (MOTOR2_ENABLE_NODE, gpios)
#define MOTOR2_ENABLE_PIN DT_GPIO_PIN (MOTOR2_ENABLE_NODE, gpios)
#define MOTOR2_ENABLE_FLAGS DT_GPIO_FLAGS (MOTOR2_ENABLE_NODE, gpios)

#define MOTOR2_NSS_NODE DT_PATH (motor2_pins, nss)
#define MOTOR2_NSS_LABEL DT_GPIO_LABEL (MOTOR2_NSS_NODE, gpios)
#define MOTOR2_NSS_PIN DT_GPIO_PIN (MOTOR2_NSS_NODE, gpios)
#define MOTOR2_NSS_FLAGS DT_GPIO_FLAGS (MOTOR2_NSS_NODE, gpios)

/****************************************************************************/

// #define MOTOR1 1
#define MOTOR2 1

void main (void)
{
        int ret{};
        const struct device *spi = device_get_binding (DT_LABEL (DT_NODELABEL (spi2)));

        if (!spi) {
                printk ("Could not find SPI driver\n");
                return;
        }

#ifdef MOTOR1
        const struct device *dir1 = device_get_binding (MOTOR1_DIR_LABEL);

        if (dir1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (dir1, MOTOR1_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_DIR_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *step1 = device_get_binding (MOTOR1_STEP_LABEL);

        if (step1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (step1, MOTOR1_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_STEP_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *enable1 = device_get_binding (MOTOR1_ENABLE_LABEL);

        if (enable1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (enable1, MOTOR1_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_ENABLE_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *nss1 = device_get_binding (MOTOR1_NSS_LABEL);

        if (nss1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (nss1, MOTOR1_NSS_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_NSS_FLAGS);

        if (ret < 0) {
                return;
        }

        gpio_pin_set (enable1, MOTOR1_ENABLE_PIN, false);
        gpio_pin_set (dir1, MOTOR1_DIR_PIN, false);

        const struct spi_cs_control spiCs1 = {.gpio_dev = nss1, .delay = 0, .gpio_pin = MOTOR1_NSS_PIN, .gpio_dt_flags = GPIO_ACTIVE_LOW};
        struct spi_config spiCfg1 = {0};

        spiCfg1.operation = SPI_WORD_SET (8) | SPI_MODE_CPOL | SPI_MODE_CPHA;
        spiCfg1.frequency = 500000U;
        spiCfg1.cs = &spiCs1;

        TMC2130Stepper driver1 (spi, &spiCfg1);

        // driver1.rms_current (600);
        // driver1.stealthChop (1);
        // driver1.microsteps (8);
        driver1.dedge (true);
        driver1.push ();
        driver1.toff (3);
        driver1.tbl (1);
        driver1.chm (true);
        driver1.hysteresis_start (4);
        driver1.hysteresis_end (-2);
        driver1.rms_current (800); // mA
        driver1.microsteps (8);
        // driver1.diag1_stall (1);
        // driver1.diag1_active_high (1);
        // driver1.coolstep_min_speed (0xFFFFF); // 20bit max
        // driver1.THIGH (0);
        // driver1.semin (5);
        // driver1.semax (2);
        // driver1.sedn (0b01);
        // driver1.sg_stall_value (0);

        gpio_pin_set (enable1, MOTOR1_ENABLE_PIN, true);
#endif

        /****************************************************************************/

#ifdef MOTOR2
        const struct device *dir2 = device_get_binding (MOTOR2_DIR_LABEL);

        if (dir2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (dir2, MOTOR2_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_DIR_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *step2 = device_get_binding (MOTOR2_STEP_LABEL);

        if (step2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (step2, MOTOR2_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_STEP_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *enable2 = device_get_binding (MOTOR2_ENABLE_LABEL);

        if (enable2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (enable2, MOTOR2_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_ENABLE_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *nss2 = device_get_binding (MOTOR2_NSS_LABEL);

        if (nss2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (nss2, MOTOR2_NSS_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_NSS_FLAGS);

        if (ret < 0) {
                return;
        }

        gpio_pin_set (enable2, MOTOR2_ENABLE_PIN, false);
        gpio_pin_set (dir2, MOTOR2_DIR_PIN, false);

        const struct spi_cs_control spiCs2 = {.gpio_dev = nss2, .delay = 0, .gpio_pin = MOTOR2_NSS_PIN, .gpio_dt_flags = GPIO_ACTIVE_LOW};
        struct spi_config spiCfg2 = {0};

        spiCfg2.operation = SPI_WORD_SET (8) | SPI_MODE_CPOL | SPI_MODE_CPHA;
        spiCfg2.frequency = 500000U;
        spiCfg2.cs = &spiCs2;

        TMC2130Stepper driver2 (spi, &spiCfg2);

        // driver2.rms_current (600);
        // driver2.stealthChop (1);
        // driver2.microsteps (8);
        driver2.dedge (true);
        driver2.push ();
        driver2.toff (3);
        driver2.tbl (1);
        driver2.chm (true);
        driver2.hysteresis_start (4);
        driver2.hysteresis_end (-2);
        driver2.rms_current (800); // mA
        driver2.microsteps (8);
        // driver2.diag1_stall (1);
        // driver2.diag1_active_high (1);
        // driver2.coolstep_min_speed (0xFFFFF); // 20bit max
        // driver2.THIGH (0);
        // driver2.semin (5);
        // driver2.semax (2);
        // driver2.sedn (0b01);
        // driver2.sg_stall_value (0);

        gpio_pin_set (enable2, MOTOR2_ENABLE_PIN, true);
#endif

        bool stepState = true;

        while (1) {
#ifdef MOTOR1
                gpio_pin_set (step1, MOTOR1_STEP_PIN, (int)stepState);
#endif

#ifdef MOTOR2
                gpio_pin_set (step2, MOTOR2_STEP_PIN, (int)stepState);
#endif
                stepState = !stepState;
                k_usleep (100);
        }
}
