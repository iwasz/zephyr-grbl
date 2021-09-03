# Project goals
* [ ] Port GRBL to Zephyr and make it to work with the UGS on my own hardware. 
* [ ] Be able to draw complex shapes and as cleanly as [the original AxiDraw](https://www.youtube.com/watch?v=5492ZjivAQ0&t=27s).
* [ ] Emulate serial port over USB.
* [ ] Display with menu and whatnot. Like on Prusa printer. 
* [ ] Implement printing from an SD card.

# Hardware
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

# Software
* [x] Default clock configuration (H7) results in 96MHz only, while the CPU is 600MHz capable I think. EDIT : Using F4 @ 168MHz
* [x] Baudrate should be configured in dts file, not using a macro. EDIT : using CDC ACM.
* [x] Refactor serial code to use Zephyr's ring buffers. Send and receive more.
* [ ] ~~DMA configuration for usart3 does not work (program faults). I used usart3 as I did in H7, but the realized the usart3 was also used for the console there. In F446 console is attached to usart2. So this mismatch likely caused a problem.~~ EDIT : using interrupt API instead of the Async one.
  * [ ] ~~I would change the grbl uart to usart3, and turn the Zephyr console on on the usart2.~~ EDIT : using the CDC ACM.
* [ ] Stall pins configuration does not work (program faults).
* [x] `k_usleep (100)` results in 200µs delay. But 10ms works fine, so something with resolution.
* [ ] Refactor (clean up) the hw_timer_driver. Change name to something more descriptive like timer_callback or sth.
* [ ] UGS hangs (I mean the transfer stalls) during second upload of the circle.ng file. Fix this. Provide seamless cooperation between the two.
  * [ ] GRBL hangs in a loop in deps/grbl/grbl/motion_control.c@61 . 
    * [ ] Jog left, jog right, input the circle program line by line. At some point it'll hang.
    * [ ] No jog at the beginning, only circle program line by line. Make a mistake along the way (input only a half of a line or something). Continue.
    * Note : when compiled with -O2 it once again works. WHY!?
    * [ ] Pause the execution after a hang, and inspect tha main stack. Turn the stack filling with a 0xaa pattern on.
* [ ] Resolve all the warnings.