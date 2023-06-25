/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "stepperDriverSettings.h"
#include "zephyrGrblPeripherals.h"
#include <TMC2130Stepper.h>

namespace drv {

void init ()
{
        spi_config spiCfg1{};
        spiCfg1.operation = SPI_WORD_SET (8) | SPI_MODE_CPOL | SPI_MODE_CPHA;
        spiCfg1.frequency = 500000U;
        spiCfg1.cs = {.gpio = nssX, .delay = 0};

        // Based on Prusa i3 MK3 settings.
        TMC2130Stepper driver1 (spi, &spiCfg1);

        const int RUNNING_CURRENT = 16; // Prusa has 16 for X and 20 for Y here.
        const int HOLDING_CURRENT = 1;
        const int MICRO_STEPS = 8; // TODO 256 µ steps?
        const bool I_SCALE_ANALOG = false;

        // TODO suboptimal. Following bunch of setting sould be stored using only 1 32bit SPI write to the CHOPCONF register.
        driver1.toff (3);
        driver1.hysteresis_start (5);
        driver1.hysteresis_end (1);
        driver1.fd (0);
        driver1.disfdcc (0);
        driver1.rndtf (0);
        driver1.chm (0);
        driver1.tbl (2);

        if (RUNNING_CURRENT <= 31) {
                driver1.vsense (1);
        }
        else {
                driver1.vsense (0);
        }

        driver1.vhighfs (0);
        driver1.vhighchm (0);
        driver1.sync (0);
        driver1.microsteps (MICRO_STEPS);
        driver1.intpol ((MICRO_STEPS == 256) ? (0) : (1));
        driver1.dedge (1);
        driver1.diss2g (0);
        driver1.I_scale_analog (I_SCALE_ANALOG);

        /*--------------------------------------------------------------------------*/

        if (RUNNING_CURRENT <= 31) {
                driver1.ihold (HOLDING_CURRENT & 0x1f);
                driver1.irun (RUNNING_CURRENT & 0x1f);
        }
        else {
                driver1.ihold ((HOLDING_CURRENT >> 1) & 0x1f);
                driver1.irun ((RUNNING_CURRENT >> 1) & 0x1f);
        }

        /*--------------------------------------------------------------------------*/

        driver1.power_down_delay (0);

        /*--------------------------------------------------------------------------*/

        driver1.COOLCONF (3 << 16);
        driver1.coolstep_min_speed (430); // Silent -> 0, normal 430

#define TMC2130_GCONF_NORMAL 0x00000000 // spreadCycle
#define TMC2130_GCONF_SGSENS 0x00003180 // spreadCycle with stallguard (stall activates DIAG0 and DIAG1 [pushpull])
#define TMC2130_GCONF_SILENT 0x00000004 // stealthChop
        driver1.GCONF (TMC2130_GCONF_SGSENS);

        /*--------------------------------------------------------------------------*/

        driver1.pwm_ampl (230); // X 230, Y == 235
        driver1.pwm_grad (2);
        driver1.pwm_freq (2);
        driver1.pwm_autoscale (1);
        driver1.pwm_symmetric (0);
        driver1.freewheel (0);

        /*--------------------------------------------------------------------------*/

        driver1.TPWMTHRS (0);

        gpio_pin_set_dt (&enableX, true);

        /****************************************************************************/

        spi_config spiCfg2{};
        spiCfg2.operation = SPI_WORD_SET (8) | SPI_MODE_CPOL | SPI_MODE_CPHA;
        spiCfg2.frequency = 500000U;
        spiCfg2.cs = {.gpio = nssY, .delay = 0};

        TMC2130Stepper driver2 (spi, &spiCfg2);

        driver2.toff (3);
        driver2.hysteresis_start (5);
        driver2.hysteresis_end (1);
        driver2.fd (0);
        driver2.disfdcc (0);
        driver2.rndtf (0);
        driver2.chm (0);
        driver2.tbl (2);

        if (RUNNING_CURRENT <= 31) {
                driver2.vsense (1);
        }
        else {
                driver2.vsense (0);
        }

        driver2.vhighfs (0);
        driver2.vhighchm (0);
        driver2.sync (0);
        driver2.microsteps (MICRO_STEPS);
        driver2.intpol ((MICRO_STEPS == 256) ? (0) : (1));
        driver2.dedge (1);
        driver2.diss2g (0);
        driver2.I_scale_analog (I_SCALE_ANALOG);

        /*--------------------------------------------------------------------------*/

        if (RUNNING_CURRENT <= 31) {
                driver2.ihold (HOLDING_CURRENT & 0x1f);
                driver2.irun (RUNNING_CURRENT & 0x1f);
        }
        else {
                driver2.ihold ((HOLDING_CURRENT >> 1) & 0x1f);
                driver2.irun ((RUNNING_CURRENT >> 1) & 0x1f);
        }

        /*--------------------------------------------------------------------------*/

        driver2.power_down_delay (0);

        /*--------------------------------------------------------------------------*/

        driver2.COOLCONF (3 << 16);
        driver2.coolstep_min_speed (430); // Silent -> 0, normal 430
        driver2.GCONF (TMC2130_GCONF_SGSENS);

        /*--------------------------------------------------------------------------*/

        driver2.pwm_ampl (230); // X 230, Y == 235
        driver2.pwm_grad (2);
        driver2.pwm_freq (2);
        driver2.pwm_autoscale (1);
        driver2.pwm_symmetric (0);
        driver2.freewheel (0);

        /*--------------------------------------------------------------------------*/

        driver2.TPWMTHRS (0);
        gpio_pin_set_dt (&enableY, true);
}

} // namespace drv