# What this is
A GRBL ported to Zephyr RTOS with some additional features like software CDC ACM serial port and SD card support. Developed for my CoreXY pen ploter, so some features of GRBL are commented out. Please create an issue or propose a pull request if you are interested in more functionality (list of unsupported bits is below). The greatest benefit of having the GRBL ported to Zephyr is the ability of running it on variety of platforms with way more powerful CPUs than the original Arduino. As of now this project is targeting the [STM32F405RG](https://www.st.com/en/microcontrollers-microprocessors/stm32f405rg.html) (earlier tests were performed on the [STM32F446RE](https://www.st.com/en/microcontrollers-microprocessors/stm32f446re.html)) and with minor modifications in the *boards* directory or *overlay* file (or adding either of those to be precise) it should work on different STM32-s as well. 

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
-----
-----
-----

# Topics to mention in a write up
* Software
  * Out of tree driver for timer callback.
  * TMC driver?
  * GRBL obviously.

