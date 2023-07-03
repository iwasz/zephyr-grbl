# Links
* https://forum.prusaprinters.org/forum/original-prusa-i3-mk3s-mk3-hardware-firmware-and-software-help/tmc2130-driver-infos-and-modifications/

# Unable to run the 'circle.ng' example
Jogging under UGS worked, but running the [example code](https://diymachining.com/g-code-example/) resulted in various errors which looked like (number of those messages was also changing from run to run):

```
[Error] An error was detected while sending 'G02X0.Y0.5I0.5J0.F2.5': (error:33) Motion command target is invalid. Streaming has been paused.
**** The communicator has been paused ****

**** Pausing file transfer. ****
```

Building the source with -O2 resolved the issue, so clearly there is some problem with inefficient UART communication.

# TMC 2130
spreadCycle is one of chopper algorithms:
* 4 phases : on, slow decay, fast decay, second slow decay
* blanking time masks the spikes.
* chopper frequency

# GRBL
The GRBL is cloned from [gnea /grbl as found here](https://github.com/gnea/grbl/wiki).

There are two timers used : TIMER1 which works in so-called "normal" mode which simply is an output compare configuration. Its frequency can be changed using OCR1A register. This timer fires an ISR which is the workhorse of the entire setup. It consumes what is called a segment (segments are simply short straight sections of some bigger primitive like arc, circle, or whatever) from a buffer if it does not have one currently, and based on this information it performs Bresenham's line algorithm to figure out which motor to step. TIMER0 is only used to turn off the step pin after some period of time.

The Bresenham is computed in every TIMER1 tick, then the step pulses are generated if needed. Assuming that motor 1 spins at velocity *w*, and motor 2 two times slower, the step pulses for motor 1 would be generated in every ISR call, but only once in every two ISR calls for motor 2. If the movement is slow, then the AMASS thing kicks in, and doubles or quadruples (or more) the Bresenham steps while retaining the motor steps.

Terminology:
* **probing** - this is when a machine tries to sense the material underneath. It slowly lowers the probe (like a metal needle) which upon touching the metal workpiece closes the circuit.
* **jogging** - moving the tool manually without any G-code program. There are special controls for this in the UGS.
* **block** - this is a planner entity. It has higher level of abstraction than the segment. A block can have many segments.
* **segment** - a stepper motor (steppr.c) entity (`st_block_t`). This is the most atomic line the motor can "draw" or follow. It has a direction, a number of steps and `step_event_count` which I am uncertain what it is.

## Workflow
I'm referring to my modified version, so some (not many) function or data names can be altered as compared to the original gnea/grbl.


Suspicious:
```
deps/gnea-grbl/grbl/gcode.c
deps/gnea-grbl/grbl/jog.c
deps/gnea-grbl/grbl/motion_control.c
deps/gnea-grbl/grbl/planner.c
deps/gnea-grbl/grbl/protocol.c
deps/gnea-grbl/grbl/serial.c
deps/gnea-grbl/grbl/limits.c
```

Blocking:
* Obvious:
  * `limits_go_home` in *limits.c*. `k_yield` added.
  * `protocol_main_loop`  in *protocol.c*. `k_yield` added in `protocol_execute_realtime` -> `protocol_exec_rt_system`.
  * `protocol_buffer_synchronize`
  * `delay_sec`, but it allready has `k_sleep` inside.
  * `mc_probe_cycle` - few places. Not so importans as I haven't ported the probing stuff.
  * `mc_line` frequently called.
  * `mc_line` -> limits_soft_check
  * limits_go_home


Important ISRs:
* *serial.c* ---> `rxRingBuf` ---> `serial_read` (access). Access has to be synchronized.
* *stepper.c*





uartInterruptHandler



## My complaints about GRBL
* Tremendous function complexity. For instance `gc_execute_line` has well over 1000 lines of code.


# Manual tests
* [x] Obviously check if SWD works.
* [x] Serial console
* [ ] USB - check if CDC ACM works
* [ ] OLED
* [ ] Steppers
* [ ] Servo