# How to build
```
rm -Rf build
mkdir build
cmake -B build -G Ninja -b -DBOARD=stm32f746g_disco -DBOARD_ROOT=. .
cmake -B build -G Ninja -b -DBOARD=nucleo_h743zi -DBOARD_ROOT=. .
cmake -B build -G Ninja -b -DBOARD=nucleo_l476rg -DBOARD_ROOT=. .
ninja -C build
```

Use ninja or IDE to build. Upload with `west flash` and after this initial upload GDB from vscode also sohould work.

* I copied `zephyrproject/zephyr/boards/arm/stm32f746g_disco` to my project to be able to modify the `stm32f746g_disco.dts` file.

./

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

# HW TODO
* Kareta
  * [ ] Wytłoczenia na łożyska w karecie. Kiedy się ją skręci, łożyska się nie kręca kiedy się skręci mocno karetę.
  * [ ] Problem z kablami. Albo wywalić switche, albo jakoś to poprawić.
* Pen holder
  * [x] Body3 łamie się jak się je wsadza. Grubsze od tyłu.
  * [x] Sprężyna ma być montowana na pręcik 1.4 (drut ocynk).
  * [x] Sprężyna musi mieć dłuższe "nóżki", bo się blokuje.
  * [x] Montowanie sprężyny do servo. Jak?
  * [x] Łożyska i pręty : to ciężko chodzi. Nie wiem czemu.
  * [x] Montowanie servo : nie da się przełożyć kabla (częściowo próbowałem naprawić, ale nie jestem zadowolony).
  * [x] Servo majta się na boki. Slot powinien być dopasowany.
  * [x] Może zamiast wkrętów uzyć śrubek 2mm

# SW TODO
* [ ] Default clock configuration (H7) results in 96MHz only, while the CPU is 600MHz capable I think.
* [ ] Baudrate should be configured in dts file, not using a macro.
* [ ] Refactor serial code to use Zephyr's ring buffers. Send and receive more 
* [ ] DMA configuration for usart3 does not work (program faults). I used usart3 as I did in H7, but the realized the usart3 was also used for the console there. In F446 console is attached to usart2. So this mismatch likely caused a problem.
  * [ ] I would change the grbl uart to usart3, and turn the Zephyr console on on the usart2.
* [ ] Stall pins configuration does not work (program faults).
* [x] `k_usleep (100)` results in 200µs delay. But 10ms works fine, so something with resolution.



# ~~GRBL main modifications~~
* "invert" logic removed from GRBL in favor of Zephyr's device tree configuration, where one can use `GPIO_ACTIVE_LOW` and `GPIO_ACTIVE_HIGH` directives.
* Kernel timers used instead of AVR's `TIMER1` and `TIMER0`.

# GRBL
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

==Compilation, DTS==


const int up_irqn = DT_IRQ_BY_IDX(DT_PARENT(DT_DRV_INST(0)), 0, irq);
		    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
const int up_priority = DT_IRQ_BY_IDX(DT_PARENT(DT_DRV_INST(0)), 0, priority);
                        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'DT_N_INST_0_st_stm32_hw_timer_PARENT_IRQ_IDX_0_VAL_irq' undeclared (first use in this function)
'DT_N_INST_0_st_stm32_hw_timer_PARENT_IRQ_IDX_0_VAL_priority' undeclared (first use in this function)

Nie było w ogóle zdefiniowanego urządzenia w overlay.

-----

'DT_N_S_soc_S_timers_40001800_S_hw_timer' undeclared (first use in this function); did you mean 'DT_N_S_soc_S_timers_40001800_S_hw_timer_ORD'?
Złe użycie makra DT_DRV_INST(0). Zamiast DT_IRQ_BY_IDX(DT_PARENT(DT_DRV_INST(0)) użyłem (dla testu) samego DT_DRV_INST(0).

-----

'DT_N_S_soc_S_timers_40001800_S_hw_timer_P_st_prescaler' undeclared here (not in a function); did you mean 'DT_N_S_soc_S_timers_40001800_S_pwm_P_st_prescaler'?
Nie widzi pliku z bindingsami (modules/hw_timer/dts/bindings/hw_timer/st,stm32-hw-timer.yaml w moim przypadku). Kiedy go umieściłem w odpowiednim miejscu to ten problem znikł. Jednak nie wiem jak łatwo sprawdzić czy system widzi moje bindings, czy nie.

==Linking==

undefined reference to `hw_timer_set_update_callback'
undefined reference to `hw_timer_pin_set_usec'

Nie dodałem pliku nagłowkowego.