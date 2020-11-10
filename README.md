# How to build
```
rm -Rf build
mkdir build
cmake -B build -G Ninja -b -DBOARD=stm32f746g_disco -DBOARD_ROOT=. .
ninja -C build
```

Use ninja or IDE to build. Upload with `west flash` and after this initial upload GDB from vscode also sohould work.

* I copied `zephyrproject/zephyr/boards/arm/stm32f746g_disco` to my project to be able to modify the `stm32f746g_disco.dts` file.