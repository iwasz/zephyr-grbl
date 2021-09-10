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
include_directories (deps/grbl2/grbl)

add_definitions("-DDEFAULTS_ZEPHYR_GRBL_PLOTTER")

target_sources(app PRIVATE 
    src/main.cc
    src/zephyrGrblPeripherals.cc
    src/stepperDriverSettings.cc

    # deps/TMC2130Stepper/src/source/SW_SPI.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_CHOPCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_COOLCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_DRV_STATUS.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_GCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_IHOLD_IRUN.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_MACROS.h
    deps/TMC2130Stepper/src/source/TMC2130Stepper_PWMCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper.cpp

    deps/grbl2/grbl/coolant_control.c
    deps/grbl2/grbl/eeprom.c    
    deps/grbl2/grbl/gcode.c
    deps/grbl2/grbl/jog.c
    deps/grbl2/grbl/limits.c        
    deps/grbl2/grbl/main.c
    deps/grbl2/grbl/motion_control.c
    deps/grbl2/grbl/nuts_bolts.c
    deps/grbl2/grbl/planner.c
    deps/grbl2/grbl/print.c
    deps/grbl2/grbl/probe.c
    deps/grbl2/grbl/protocol.c
    deps/grbl2/grbl/report.c   
    deps/grbl2/grbl/serial.c
    deps/grbl2/grbl/settings.c
    deps/grbl2/grbl/spindle_control.c
    deps/grbl2/grbl/stepper.c
    deps/grbl2/grbl/system.c
)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_cfb_font_for_target(app fonts/oldschool-black-format.png ${gen_dir}/cfb_font_oldschool.h 7 8 --first 32 --last 126)
