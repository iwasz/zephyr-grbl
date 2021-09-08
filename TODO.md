# Project goals
* [ ] Port GRBL to Zephyr and make it work with the UGS on my own hardware. 
* [ ] Be able to draw complex shapes and as cleanly as [the original AxiDraw](https://www.youtube.com/watch?v=5492ZjivAQ0&t=27s).
* [x] Emulate serial port over USB.
* [ ] Display a menu and whatnot. Like on Prusa printer. 
* [ ] Implement printing from a SD card.

# Hardware
* Kareta
  * [ ] Wytłoczenia na łożyska w karecie. Kiedy się ją skręci, łożyska się nie kręca kiedy się skręci mocno karetę.
  * [ ] Problem z kablami. Albo wywalić switche, albo jakoś to poprawić.
  * [ ] It's almost impossible to stick the belt into place. The slots are to tight!
  * [ ] PCB for edge switches.
* Pen holder
  * [x] Body3 łamie się jak się je wsadza. Grubsze od tyłu.
  * [x] Sprężyna ma być montowana na pręcik 1.4 (drut ocynk).
  * [x] Sprężyna musi mieć dłuższe "nóżki", bo się blokuje.
  * [x] Montowanie sprężyny do servo. Jak?
  * [x] Łożyska i pręty : to ciężko chodzi. Nie wiem czemu.
  * [x] Montowanie servo : nie da się przełożyć kabla (częściowo próbowałem naprawić, ale nie jestem zadowolony).
  * [x] Servo majta się na boki. Slot powinien być dopasowany.
  * [x] Może zamiast wkrętów uzyć śrubek 2mm
  * [ ] The spring broke off after 30 minutes of work time. Make it thicker, or **redesign**.
  * [ ] Too much friction on the bearings which are of poor quality. Replace with slide bearnigs? Replace the rods? Redesign from ground up.

# Software
* [x] Default clock configuration (H7) results in 96MHz only, while the CPU is 600MHz capable I think. EDIT : Using F4 @ 168MHz
* [x] Baudrate should be configured in dts file, not using a macro. EDIT : using CDC ACM.
* [x] Refactor serial code to use Zephyr's ring buffers. Send and receive more.
* [ ] ~~DMA configuration for usart3 does not work (program faults). I used usart3 as I did in H7, but the realized the usart3 was also used for the console there. In F446 console is attached to usart2. So this mismatch likely caused a problem.~~ EDIT : using interrupt API instead of the Async one.
  * [ ] ~~I would change the grbl uart to usart3, and turn the Zephyr console on on the usart2.~~ EDIT : using the CDC ACM.
* [ ] Stall pins configuration does not work (program faults).
* [x] `k_usleep (100)` results in 200µs delay. But 10ms works fine, so something with resolution.
* [ ] Refactor (clean up) the hw_timer_driver. Change name to something more descriptive like timer_callback or sth.
* [x] UGS hangs (I mean the transfer stalls) during second upload of the circle.ng file. Fix this. Provide seamless cooperation between the two.
  * [x] GRBL hangs in a loop in deps/grbl/grbl/motion_control.c@61 . 
    * [x] Jog left, jog right, input the circle program line by line. At some point it'll hang.
    * [x] No jog at the beginning, only circle program line by line. Make a mistake along the way (input only a half of a line or something). Continue.
    * Note : when compiled with -O2 it once again works. WHY! It is NOT a stack overflow problem because I have the stack protection turned ON. EDIT : this was all by fault, I removed (I should say : I didn't port this portion from AVR to Zephyr) the IRQ critical section barriers in `system.c` `system_set_exec_state_flag()` (and in other functions there as well).
* [ ] Resolve all the warnings.
* [x] PWM setting in the timer ISR works slow. Why? 
  * ~~Seems like `pwm_pin_set_usec` takes exactly 100µs to execute.~~ 
  * When run from the main thread `pwm_pin_set_usec` works in no time. ~~I'm starting to think, that my problems were due to misconfigured `DEFAULT_Z_MAX_RATE`. It was way to high, and the FW tried to update the Z axis to frequently.~~
  * **Something** is causing `pwm_pin_set_usec` to work very slowly when run from an ISR. Updating the TIM2->CCR1 directly works fine.
  * It was due to misconfiguration. Both servo PWM and the main timer callback were using the same timer and channel.
* [ ] Now that the PWM and the servo are resolved, another problem occured : Z moves too slowly and the pen leaves gaps (before first segment drawn or after the last - not sure).
* [ ] I may be wrong but there's something wrong with the scale of the plot when drawn in inches. In mm everything seems to be OK (sphere dia ~146mm).
* [ ] When compiled with -O0 and ran under GDB, the quality (accuracy) deteriorates drammaticaly. Why is it **so much** of a change?
* [ ] **Maybe**, just maybe refactor the TMC2130 code to a Zephyr driver.
* [ ] **Maybe** join the two GitHub projects (grbl and zephyr-grbl-plotter) into one. Easier for maintenance.
* [ ] Speeds in `default.h` are probably in wrong units. Default there equals to 10mm per minute. This should result in very slow movement, but in reality the device moves fast.
  * [ ] Feed rate is not working at all as it seems? It is ignored in G commands.
