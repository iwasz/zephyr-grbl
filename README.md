# See also
* [TODO](TODO.md) list of things to do.
* [My notes](NOTES.md) on problems I've had. 

# How to build
```
rm -Rf build
cmake -B build -G Ninja -b -DBOARD=stm32f746g_disco -DBOARD_ROOT=. .
cmake -B build -G Ninja -b -DBOARD=nucleo_h743zi -DBOARD_ROOT=. .
cmake -B build -G Ninja -b -DBOARD=nucleo_l476rg -DBOARD_ROOT=. .
ninja -C build
```

Use ninja or IDE to build. Upload with `west flash` and after this initial upload GDB from vscode also sohould work.

* I copied `zephyrproject/zephyr/boards/arm/stm32f746g_disco` to my project to be able to modify the `stm32f746g_disco.dts` file.

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
The GRBL is cloned from [gnea /grbl as found here](https://github.com/gnea/grbl/wiki).

There are two timers : TIMER1 which works in so-called "normal" mode which simply is an output compare configuration. Its frequency can be changed using OCR1A register. This timer fires an ISR which is the workhorse of the entire setup. It consumes what is called a segment (segments are simply short straight sections of some bigger primitive like arc, circle, or whatever) from a buffer if it does not have one currently, and based on this information it performs Bresenham's line algorithm to figure out which motor to step. TIMER0 is only used to turn off the step pin after some period of time.

The Bresenham is computed in every TIMER0 tick, then the step pulses are generated if needed. Assuming that motor 1 spins at velocity *w*, and motor 2 two times slower, the step pulses for motor 1 would be generated in every ISR call, but only once in every two ISR calls for motor 2. If the movement is slow, then the AMASS thing kicks in, and doubles or quadruples (or more) the Bresenham steps while retaining the motor steps.

Terminology:
* probing - this is when a machine tries to sense the material underneath. It slowly lowers the probe (like a metal needle) which upon touching the metal workpiece closes the circuit.
* jogging - moving the tool manually without any G-code program. There are special controls for this in the UGS.

# What has been done in order to port
* [x] Zephyr PWM module was modified to accept a timer output compare ISR callback.
* [x] AVR GPIO registers code ported to zephyr 
* [x] Gpio inverting functionality stripped down as from now the Zephyr's device tree configuration is used for this.
* ~~Logging ported to Zephyr.~~ Rolled back.
* [ ] Port uart code to zephyr.
  * [ ] Make use of new async uart functionality merged recently to zephyr. [Analogous to this](https://github.com/zephyrproject-rtos/zephyr/pull/30917/commits/a62711bd260fea80948f668d35b05452bd26e95f). 
    * [x] ~~Modify the overlay.~~
    * [x] ~~Turn DMA on in Kconfig.~~
    * This does not work, as H7 has DMA-V1 + DMAMUX, which is not implemented in Zephyr (this combination). This is my understanding. Therefore I'll try my luck with L476
    * [ ] L476 overlay.

