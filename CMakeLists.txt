# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky)

include_directories (deps/TMC2130Stepper/src)

target_sources(app PRIVATE 
    src/main.cc

    # deps/TMC2130Stepper/src/source/SW_SPI.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_CHOPCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_COOLCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_DRV_STATUS.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_GCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_IHOLD_IRUN.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper_MACROS.h
    deps/TMC2130Stepper/src/source/TMC2130Stepper_PWMCONF.cpp
    deps/TMC2130Stepper/src/source/TMC2130Stepper.cpp
)
