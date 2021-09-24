# SPDX-License-Identifier: Apache-2.0

list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/hw_timer
)

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
include(${ZEPHYR_BASE}/cmake/cfb.cmake NO_POLICY_SCOPE)
project(plotter)

include_directories (src)
include_directories (deps/TMC2130Stepper/src)
include_directories (deps/etl/include/)
include_directories (deps/gsl-lite/include)
include_directories (deps/gnea-grbl) # Warning! It has limits.h file which interferes with libc
include_directories (deps/compile-time-regular-expressions/include)
include_directories (deps/libstate/src)

add_definitions("-DDEFAULTS_ZEPHYR_GRBL_PLOTTER")

target_sources(app PRIVATE 
    src/main.cc
    src/zephyrGrblPeripherals.cc
    src/stepperDriverSettings.cc
    src/sdCard.cc

    # deps/TMC2130Stepper/src/source/SW_SPI.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_CHOPCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_COOLCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_DRV_STATUS.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_GCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_IHOLD_IRUN.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_MACROS.h
    deps/TMC2130Stepper/src/source/TMC2130Stepper_PWMCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper.cpp

    deps/gnea-grbl/grbl/coolant_control.c
    deps/gnea-grbl/grbl/eeprom.c    
    deps/gnea-grbl/grbl/gcode.c
    deps/gnea-grbl/grbl/jog.c
    deps/gnea-grbl/grbl/limits.c        
    deps/gnea-grbl/grbl/main.c
    deps/gnea-grbl/grbl/motion_control.c
    deps/gnea-grbl/grbl/nuts_bolts.c
    deps/gnea-grbl/grbl/planner.c
    deps/gnea-grbl/grbl/print.c
    deps/gnea-grbl/grbl/probe.c
    deps/gnea-grbl/grbl/protocol.c
    deps/gnea-grbl/grbl/report.c   
    deps/gnea-grbl/grbl/serial.c
    deps/gnea-grbl/grbl/settings.c
    deps/gnea-grbl/grbl/spindle_control.c
    deps/gnea-grbl/grbl/stepper.c
    deps/gnea-grbl/grbl/system.c
)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_cfb_font_for_target(app fonts/oldschool-black-format.png ${gen_dir}/cfb_font_oldschool.h 7 8 --first 32 --last 126)
