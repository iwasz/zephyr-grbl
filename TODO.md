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
* [ ] Default clock configuration (H7) results in 96MHz only, while the CPU is 600MHz capable I think.
* [ ] Baudrate should be configured in dts file, not using a macro.
* [ ] Refactor serial code to use Zephyr's ring buffers. Send and receive more 
* [ ] DMA configuration for usart3 does not work (program faults). I used usart3 as I did in H7, but the realized the usart3 was also used for the console there. In F446 console is attached to usart2. So this mismatch likely caused a problem.
  * [ ] I would change the grbl uart to usart3, and turn the Zephyr console on on the usart2.
* [ ] Stall pins configuration does not work (program faults).
* [x] `k_usleep (100)` results in 200µs delay. But 10ms works fine, so something with resolution.