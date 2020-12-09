/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "stepper-drivers.h"
#include "mcu-peripherals.h"
#include <TMC2130Stepper.h>

namespace drv {

void init ()
{
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

        driver1.sgt (10);
        driver1.sfilt (false);

        driver1.diag1_stall (true);
        driver1.diag1_active_high (true);

        driver1.coolstep_min_speed (0xFFFFF); // 20bit max
        driver1.THIGH (0);
        driver1.semin (5);
        driver1.semax (2);
        driver1.sedn (0b01);
        driver1.sg_stall_value (0);

        gpio_pin_set (enable1, MOTOR1_ENABLE_PIN, true);

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

        driver2.diag1_stall (1);
        driver2.diag1_active_high (1);
        driver2.coolstep_min_speed (0xFFFFF); // 20bit max
        driver2.THIGH (0);
        driver2.semin (5);
        driver2.semax (2);
        driver2.sedn (0b01);
        driver2.sg_stall_value (0);

        gpio_pin_set (enable2, MOTOR2_ENABLE_PIN, true);
}

} // namespace drv