# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky)

include_directories (src)
include_directories (deps/TMC2130Stepper/src)
include_directories (deps/etl/include/)
include_directories (deps/gsl-lite/include)
include_directories (deps/grbl/grbl)

target_sources(app PRIVATE 
    src/main.cc
    src/mcu-peripherals.cc
    src/stepper-drivers.cc

    # deps/TMC2130Stepper/src/source/SW_SPI.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_CHOPCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_COOLCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_DRV_STATUS.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_GCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_IHOLD_IRUN.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_MACROS.h
    deps/TMC2130Stepper/src/source/TMC2130Stepper_PWMCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper.cpp

    # Not needed, leaved commented to avoid confusion
    # deps/grb/grbl/coolant_control.c
    # src/coolant_control_stub.c

    # # NVM
    # # deps/grbl/grbl/eeprom.c
    # src/eeprom_stub.c
    # deps/grbl/grbl/gcode.c
    # deps/grbl/grbl/jog.c
    
    # # Pins, interrupts
    # # deps/grbl/grbl/limits.c
    # src/limits_stub.c
    
    # deps/grbl/grbl/main.c
    # deps/grbl/grbl/motion_control.c
    # deps/grbl/grbl/nuts_bolts.c
    # deps/grbl/grbl/planner.c
    # deps/grbl/grbl/print.c

    # # Pins, interrupts
    # # deps/grbl/grbl/probe.c
    # src/probe_stub.c

    # deps/grbl/grbl/protocol.c
    # deps/grbl/grbl/report.c
    
    # # UART, iunterrupts
    # # deps/grbl/grbl/serial.c
    # deps/grbl/grbl/serial.cc
    
    # # Undeclared macros and variables.
    # # deps/grbl/grbl/settings.c

    # # PWM, probably not necessary
    # # deps/grbl/grbl/spindle_control.c
    # src/spindle_control_stub.c
    
    # # Pins, interrupts, timer. This is the file where everything happens.
    # deps/grbl/grbl/stepper.c
        
    # # Pins, interrupts. SREG (interrupt status)
    # deps/grbl/grbl/system.c
)
