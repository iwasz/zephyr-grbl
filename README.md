# What this is
A GRBL ported to Zephyr RTOS with some additional features like software CDC ACM serial port and SD card support. Developed for my CoreXY pen ploter, so some features of GRBL are commented out. Please create an issue or propose a pull request if you are interested in more functionality (list of unsupported bits is below). The greatest benefit of having the GRBL ported to Zephyr is the ability of running it on variety of platforms with way more powerful CPUs than the original Arduino. As of now this project is targeting the [STM32F406RG](https://www.st.com/en/microcontrollers-microprocessors/stm32f405rg.html) (earlier tests were performed on the [STM32F446RE](https://www.st.com/en/microcontrollers-microprocessors/stm32f446re.html)) and with minor modifications in the *overlay* file (or adding another overlay file to be precise) it should work on different STM32-s as well. 

The only *non standard* driver this project uses is the `modules/hw_timer` and this is the only obstacle for running this on MCUs different than STM32-s. `modules/hw_timer` is derived from the `pwm` driver and its only purpose is to fire a callback(s) on a timer update event (and possibly on output compare event in the future). Other parts are implemented in stock Zephyr.

The project was written to power my [CoreXY pen plotter](https://hackaday.io/project/177237-corexy-pen-plotter), and as such has some parts not implemented (probing) and others not tested (Z-axis with stepper motor, see below). It should be a good starting point to develop something more complete that would run the variety of CNC machines. Pull requests are welcome.

* [TODO](TODO.md) list of things to do.
* [My notes](NOTES.md) on problems I've had. 
* [Hackaday.io project page](https://hackaday.io/project/177237-corexy-pen-plotter)

# How to build
Tested with Zephyr RTOS 2.6.99. Familiarize yourself with the [Zephyr RTOS build process](https://docs.zephyrproject.org/latest/getting_started/index.html) first. :


## With bootloader



## Without bootloader (only for troubleshooting)
That is when `CONFIG_BOOTLOADER_MCUBOOT=n` in the `prj.conf`.
```sh
rm -Rf build
cmake -B build -GNinja -DBOARD=plotter_f405rg -DBOARD_ROOT=. .
ninja -C build
west flash # Or use IDE, or use openocd directly
```

-----
-----
-----

# Topics to mention in a write up
* Software
  * Out of tree driver for timer callback.
  * TMC driver?
  * GRBL obviously.

# TMC 2130
spreadCycle is one of chopper algorithms:
* 4 phases : on, slow decay, fast decay, second slow decay
* blanking time masks the spikes.
* chopper frequency

# ~~GRBL main modifications~~
* "invert" logic removed from GRBL in favor of Zephyr's device tree configuration, where one can use `GPIO_ACTIVE_LOW` and `GPIO_ACTIVE_HIGH` directives.
* ~~Kernel timers used instead of AVR's `TIMER1` and `TIMER0`.~~

# GRBL
## My notes / analysis
The GRBL is cloned from [gnea /grbl as found here](https://github.com/gnea/grbl/wiki).

There are two timers used : TIMER1 which works in so-called "normal" mode which simply is an output compare configuration. Its frequency can be changed using OCR1A register. This timer fires an ISR which is the workhorse of the entire setup. It consumes what is called a segment (segments are simply short straight sections of some bigger primitive like arc, circle, or whatever) from a buffer if it does not have one currently, and based on this information it performs Bresenham's line algorithm to figure out which motor to step. TIMER0 is only used to turn off the step pin after some period of time.

The Bresenham is computed in every TIMER1 tick, then the step pulses are generated if needed. Assuming that motor 1 spins at velocity *w*, and motor 2 two times slower, the step pulses for motor 1 would be generated in every ISR call, but only once in every two ISR calls for motor 2. If the movement is slow, then the AMASS thing kicks in, and doubles or quadruples (or more) the Bresenham steps while retaining the motor steps.

Terminology:
* **probing** - this is when a machine tries to sense the material underneath. It slowly lowers the probe (like a metal needle) which upon touching the metal workpiece closes the circuit.
* **jogging** - moving the tool manually without any G-code program. There are special controls for this in the UGS.
* **block** - this is a planner entity. It has higher level of abstraction than the segment. A block can have many segments.
* **segment** - a stepper motor (steppr.c) entity (`st_block_t`). This is the most atomic line the motor can "draw" or follow. It has a direction, a number of steps and `step_event_count` which I am uncertain what it is.

## My complaints about GRBL
* Tremendous function complexity. For instance `gc_execute_line` has well over 1000 lines of code. 

## What has been done in order to port GRBL
* Added a new macro for my machine named DEFAULTS_ZEPHYR_GRBL_PLOTTER in the `config.h` and globally in the `CMakeLists.txt`. Used mostly in the `defaults.h`
* Coolant control, spindle **comented out**.
* [x] Added `extern "C" {` to most of the `*.h` files to enable C++ interoperability.
* [x] Added definitions for my machine in the `defaults.h`. 
* [ ] Eeprom support ported.
* [x] Zephyr PWM module was modified to accept a timer output compare ISR callback.
* [x] AVR GPIO registers code ported to zephyr 
* [x] Homing and limiters ported.
* [x] Probe implementation commented out.
* [x] Control switches support commented out. I'll have an OLED display and I can add those features there.
* [x] Spindle control commented out. 
* [x] ~~Gpio inverting functionality stripped down as from now the Zephyr's device tree configuration is used for this.~~
* ~~Logging ported to Zephyr.~~ Rolled back.
* [x] Port uart code to zephyr.
  * [ ] ~~Make use of new async uart functionality merged recently to zephyr.~~ [Analogous to this](https://github.com/zephyrproject-rtos/zephyr/pull/30917/commits/a62711bd260fea80948f668d35b05452bd26e95f). 
    * [x] ~~Modify the overlay.~~
    * [x] ~~Turn DMA on in Kconfig.~~
    * [x] Interrupt UART API and Zephyr ring buffers.
* GRBL seems to lack any call to a `delay` function in the main loop. This prevents scheduler from switching to other lower-priority threads which in my case were : `idle`, `logging` and `shell_uart`. I've lowered the main thread priority to 14 (from 0) and added k_yeld, so other low priority threads have a chance to run now. This include logging, shell and others.