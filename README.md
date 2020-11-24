# How to build
```
rm -Rf build
mkdir build
cmake -B build -G Ninja -b -DBOARD=stm32f746g_disco -DBOARD_ROOT=. .
cmake -B build -G Ninja -b -DBOARD=nucleo_h743zi -DBOARD_ROOT=. .
ninja -C build
```

Use ninja or IDE to build. Upload with `west flash` and after this initial upload GDB from vscode also sohould work.

* I copied `zephyrproject/zephyr/boards/arm/stm32f746g_disco` to my project to be able to modify the `stm32f746g_disco.dts` file.


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
* [ ] Default clock configuration results in 96MHz only, while the CPU is 600MHz capable I think.
* [ ] 