# Project goals
* [x] Port GRBL to Zephyr and make it work with the UGS on my own hardware.
* [ ] Be able to draw complex shapes and as cleanly as [the original AxiDraw](https://www.youtube.com/watch?v=5492ZjivAQ0&t=27s).
* [x] Emulate serial port over USB.
* [ ] Display a menu and whatnot. Like on Prusa printer.
* [ ] Implement printing from a SD card.

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
* [x] **Maybe** join the two GitHub projects (grbl and zephyr-grbl-plotter) into one. Easier for maintenance.
  * [x] Rollback formating done by my clang-format for easier diff'ing.
* [x] Rollback my earlier changes that deleted the inversion implementation in GRBL.
* [ ] Settings to NVM.
  * [x] MCUboot has to be installed in order to flash partitions to work at all.

  | name            | EEPROM addr | notes                                                                |
  | --------------- | ----------- | -------------------------------------------------------------------- |
  | version         | 0           | 10                                                                   |
  | global settings | 1           | The settings. Written and read all at once. Structure : `settings_t` |
  | parameters      | 512         | "coord data" only. xxxx,yyyy,zzzz,c i.e. 3x float                    |
  | startup block   | 768         | N_STARTUP_LINE lines of g-code. String 80 (2x)                       |
  | build info      | 942         | Can be accesed using the $I command (?), string 80                   |
  | *last writable* | 1023        |

* [x] Implement limit pins. Now that the limit pins work I noticed that:
  * [x] Homing blocks other threads. Try inserting k_yield somewhere.
  * [x] Homing moves opposite direction
  * [ ] After homing it sometimes move in the X direction.
  * [ ] Homing / limit switches have too short of a travel. Fast moving axes don't have a chance to break on ~1mm distance.
* [x] Speeds in `default.h` are probably in wrong units. Default there equals to 10mm per minute. This should result in very slow movement, but in reality the device moves fast.
  * [x] Feed rate is not working at all as it seems? It is ignored in G commands.
  * [x] After resolving the problems with feed rate, when the feedrate sent to the plloter is too low (1-10) the ploter stops.
  * [ ] `settings.pulse_microseconds` is set, but it is not of any use, because there is no *output compare* callback set. It can't even be set right now.
  * [ ] The plotter is slightly slower than the desired feed rate (~25%).
  * [ ] When set to very low feed rate, the movement is jerky. For instance when feed rate is 1 mm/min the carret advances a fraction of a mm, and then stops, and repeats. Ticking sound can be heard.
    * [x] Ahhh I spent a day on this. It seems that : 1. [TMC2130 works best with 24V instead of 12](https://forum.prusaprinters.org/forum/original-prusa-i3-mk3s-mk3-hardware-firmware-and-software-help/tmc2130-driver-infos-and-modifications/). When feed from 12V, the back EMF to input signal is to high (compared to 12V supply signal driving the coils), and TMC drivers get confused. 2. (more importantly) my motors are somehow unsuitable for the TMC2130. I don't understand this, but it has something to do with the current and coil resistance.

      | Model           | rated current | coil resistance | steps | notes                         |
      | --------------- | ------------- | --------------- | ----- | ----------------------------- |
      | JK28HS32-0674   | 0.67A         | 6.2             | 200   | small one, smooth             |
      | JK42HM40-0406   | 0.4A          | 60              | 400   | nema 17 shorter, jerky, noisy |
      | 42STHM48-0406   | 0.4A          | 60              | 400   | nema 17 longer, jerky, noisy  |
      | Prusa i3 MK3 XY | 1A            | 6.5             | 200   | reference                     |
      | 17HS4401        | 1.5A          | 2.4             | 200   | popular, cheap, smooth        |
      After testing I'v decided to go with 17HS4401 as they prooven to perform smooth and silent.
* [x] Make a custom board configuration.
* [ ] Enable all the peripherals.
  * [ ] Enable OLED.
* [ ] Refactor GRBL so that it can print from the SD card and from the USB.
  * [x] `serial_read` has to be abstracted-out. In the SD card scenario it has to return the next character from the selected file.
  * [x] Main IRQ should have higher priority than the logging and shell threads. It's the most important one! Thers no point of having it with the priority so low.
  * [x] Wrap *protocol.c*s `protocol_main_loop` in a thread. Priority higher than 14. (EDIT: main thread).
  * [x] Synchronize access to ring buffers in the serial.c (after adding the thread).
  * [ ] Implenent a few useful commnads into the state machine itself (homing, reset). SD/GRBL thread
  * [ ] Implement printing from the sd card in the state machine. SD/GRBL thread.
* [ ] Implement the menu:
  * [ ] Print from file
    * [ ] List of files
  * [ ] GRBL command buttons (as a menu).
    * [ ] Feed hold
    * [ ] Cycle start
    * [ ] Reset
* [ ] Possibly implement (I mean anable in the MCUBoot) the DFU over USB.
* [ ] Some recent changes caused that shell reacts on every second keypress only. It happened about when I worked on MCUBoot.
* [ ]  The board does not seem to do POR. Haveto be manually reset.

# Mechanical
* [ ] Make a cutout for 17HS4401's cable supporting protrusion.
* [ ] Make some marking showing where is the origin of the coordinate system. This is to show to the user where to place a sheet of paper.
* Kareta
  * [ ] Wytłoczenia na łożyska w karecie. Kiedy się ją skręci, łożyska się nie kręca kiedy się skręci mocno karetę.
  * [ ] Problem z kablami. Albo wywalić switche, albo jakoś to poprawić.
  * [ ] It's almost impossible to stick the belt into place. The slots are to tight!
  * [ ] PCB for limit switches.
  * [ ] Shielded cable for limit switches & Z (as suggested by grbl docs.)
  * [ ] Low pass filter on limiter inputs (as suggested by grbl docs.)
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

# PCB
* [ ] STD14 connector for stlinkv3.
* [ ] OLED screen connector is (was?) flipped. Check & fix.