# Purpose

On DRA7xx, Boot ROM copies the first stage boot loader(MLO/SPL) from
QSPI at a conservative speed of 11 MBps. A 120 KB binary takes around
11 ms to be copied from QSPI into OCMC. Usually this is not an
issue. However there are usecases(e.g. CAN response) where we need to
have a specific functionality available in less than 100 ms.  In such
usecases, the time spent in ROM copy forms a significant portion (10%)
of the usecase time. The micro bootloader(`umlo`) built in this
project speeds up this copy.

`umlo` first sets up the QSPI interface to the maximum speed possible
on DRA7xx i.e. 76.8 MHz interface clock, Quad Mode and Mode 0
operation. Then `umlo` copies the `MLO` to the execution address in
OCMC and jumps to it.

With this change, we see the time taken to enter a 120 KB MLO reduce
from 24.5 ms to 19 ms, a saving of 5.5 ms.

Please refer to manifest.html for license information.

# Build Instructions

This tool is compiled and tested with `gcc-arm-none-eabi-4_9-2015q3`.
However any baremetal compiler supporting Cortex A15 should work.
Please make sure that you have the toolchain installed and have
`arm-none-eabi-gcc` in the path.

Run `make` to produce the required binaries.

If you are modifiying the toolchain, please ensure that the `CROSS_COMPILE`
option is set correctly in the Makefile.

# Flashing instructions

1. Flash the output file `umlo` to offset 0x0 in QSPI.

2. Flash the actual MLO from your normal build process to offset
   0x10000 (64 KB).

Nothing else needs to change.

Reboot the EVM in QSPI4 boot mode. `umlo` boots and reads the actual
MLO from offset 0x10000 into OCMC and jumps to it. You will not see
any difference in execution except a slight speed up in reading MLO.

For information on measuring time to enter MLO, please see [1]
in references.

## Undoing the changes

If you suspect a problem is being introduced due to `umlo`, you can
remove it by erasing the first 64 KB of QSPI. This will cause
Boot ROM to jump to your MLO.

# Development Notes

1. We used the peripheral boot feature of the DRA7xx Boot ROM heavily for
testing `umlo` during development. If you are customizing `umlo`,
please see [2] and [3] in references on how to use perhipheral boot
for debugging.

2. The `Makefile` has two sets of build options, one for development
   and another for release. Please switch to the development build
   options when debugging.

3. The file `main.c` contains an infinite loop function
   `wait_for_debugger()`. You can call this function at the point
   where you want to halt execution in code and single step via CCS
   from that point.


# Caveats

1. `umlo` expects that the first 512 bytes of the acutal MLO is the CH Header
   and skips it. This is the case when MLO is produced from building U-Boot.
   Please modify the code in `main.c` if this assumption is not true for the
   `MLO` from your build.

2. `umlo` does not do any SPI flash specific configuration.  `umlo`
   expects that the SPI flash has quad read mode enabled. It has only
   been tested on TI EVM's which have a Spansion flash device.

3. `umlo` is loaded to address 0x40330000 to avoid any overlap with the actual
   MLO. Please modify this address in the `Makefile` if the actual MLO runtime
   locations overlap with 0x40330000.

        CONFIG_UMLO_BASE=0x40300000

# Support

For support, please post any questions to

<https://e2e.ti.com/support/arm/automotive_processors/f/1020>

# References

1. Linux Boot Time Optimizations on DRA7xx devices

    <http://www.ti.com/lit/pdf/sprac82>

2. Using Peripheral Boot and DFU for Rapid Development on Jacinto 6 Devices

    <http://www.ti.com/lit/pdf/sprac65>

3. DRA7xx Bootswitch - Utility for Peripheral boot

    <https://git.ti.com/glsdk/dra7xx-bootswitch>

4. Please see the chapter "Initialization" in the Device TRM.
