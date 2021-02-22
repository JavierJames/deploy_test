Kien ESP32
====================
[![CircleCI](https://circleci.com/gh/kien-company/esp32-firmware.svg?style=svg&circle-token=a4793025c34847485221f9dabf03ac98753a7810)](https://circleci.com/gh/kien-company/esp32-firmware)

This repo contains the code for the ESP32 in Kien Speakers (v1).

## Setting up the environment
1. Install [ESP32 toolchain](https://esp-idf.readthedocs.io/en/latest/get-started/index.html#setup-toolchain)
2. Install [ESP-IDF](https://esp-idf.readthedocs.io/en/latest/get-started/index.html#get-esp-idf)
    1. Clone the repo (`git clone git@github.com:espressif/esp-idf.git`)
    2. Checkout to version v4.1 LTS (`git checkout v4.1`)
    3. Update submodules (`git submodule update --init --recursive`)
    4. Run the install script ('./install.sh')
    5. Run the export script ('. ./export.sh')
3. Create a file called `Makefile.local` in the project root with the following content: 
```
IDF_PATH := /absolute/path/to/esp-idf
```

## Compile and flash the target
* To compile the project run `make all`
* To flash the target run `make flash`
By default, these commands will build a satellite. **To build a subwoofer**, there are two options:
- Use `make menuconfig` and check option "Component config -> Build subwoofer"
- Append `CONFIG_IS_SUBWOOFER=1` to the aforementioned commands (e.g `make CONFIG_IS_SUBWOOFER=1 all`)

IMPORTANT NOTE: compiling the code with optimization level -0g(Debug) creates a bin file bigger than the partitions allow.
Increasing the factory partition is possible for debugging but make sure to compile with -0s(compile for size)
for production by:
- running `make menuconfig`
- go to `Compiler options --> Optimization Level -->`
- select `Optimize for size (-Os)`
Check after compilation if the size of the output bin file is less than 1M.

## Compile and run the unit tests
*Note: instructions are for Linux. It should be compatible with other OS, but it has not been tested*
* Setup the environment with `mkdir -p build/test` and `cmake ..`
* Run the tests with `make all test`. If a test fails, more details can be get by running `./tdd`

## Static analysis
The most simple option relies on [cppcheck](cppcheck.sourceforge.net). To perform the analysis run `cppcheck --template=gcc --enable=style main/`.

## EXTRA: Code style
To keep a common coding style, the files that have been modified shall be formatted before committing them using `clang-format`. To use it follow these steps:
1. Install `clang-format`
2. Run `find main/ -iname "*.cpp" -o -iname "*.hpp" -o -iname "*.c" -o -iname "*.h" | xargs clang-format -i -style=file`
Note. Many IDEs allow to do this automatically, check in the Preferences or in the avaiable Plugins.

There is also available a config file for [EditorConfig](https://editorconfig.org). Make sure you install that in your IDE.

## DMA buffer debugging
To get the free space available on the DMA buffer.
1. Replace the files i2s.c and i2s.h in the IDF folder by the ones provided in the repo.
2. when calling i2s_write_expand(), add an extra parameter. This needs to be a reference to an uint32_t.
