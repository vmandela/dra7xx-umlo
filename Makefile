#
# SPDX-License-Identifier:	BSD-3-Clause
#
CROSS_COMPILE=arm-none-eabi-
CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
LD=$(CROSS_COMPILE)gcc
OBJCOPY=$(CROSS_COMPILE)objcopy

# Build options for development
# CONFIG_UMLO_BASE=0x40300000
# CFLAGS=-O0 -g

# Build options for release
CONFIG_UMLO_BASE=0x40330000
CFLAGS=-Os -fdata-sections -ffunction-sections -mcpu=cortex-a15

OUTPUTS=umlo.elf umlo.bin umlo

DEBUG_OUTPUTS=sym_map.txt disasm.txt

all: $(OUTPUTS)
COBJS=startup.o main.o

# -Wl,--print-gc-sections Add this to debug pruned sections

umlo.elf: linker.ld $(COBJS)
	$(LD) -Wl,--defsym,CONFIG_UMLO_BASE=$(CONFIG_UMLO_BASE) \
		-T linker.ld $(COBJS) \
		-nostdlib -lgcc -o $@ -Wl,--gc-sections
	du -b $@

umlo.bin: umlo.elf
	$(OBJCOPY) -O binary $< $@
	du -b $@

umlo: umlo.bin mkimage
	./mkimage -a $(CONFIG_UMLO_BASE) $< $@
	du -b $@

mkimage: mkimage.c
	gcc mkimage.c -o mkimage

sym_map.txt: umlo.elf
	readelf -a $< > $@

disasm.txt : umlo.elf
	arm-linux-gnueabihf-objdump -S $< > $@

clean:
	rm -f *.o
	rm -f $(OUTPUTS) $(DEBUG_OUTPUTS)
	rm -f mkimage
