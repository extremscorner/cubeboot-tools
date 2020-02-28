
CROSS=powerpc-linux-
CC=$(CROSS)gcc
OBJDUMP=$(CROSS)objdump
OBJCOPY=$(CROSS)objcopy

HOSTCC = gcc

DOLTOOL = doltool

CFLAGS :=


apploader_entry_point = 81200000


apploader_C_SRCS = apploader.c
apploader_C_OBJS = $(patsubst %.c, %.o, $(apploader_C_SRCS))

apploader_SRCS = $(apploader_C_SRCS)
apploader_OBJS = $(apploader_C_OBJS) ../common/lib.o ../common/misc.o


all: apploader.bin


entry-point = $(firstword $(shell $(OBJDUMP) --all-headers "$(1)" | grep "start address" | sed -e 's/^.*0x//'))

entry_point_check: apploader.elf
	@$(if $(findstring $(apploader_entry_point),$(call entry-point,$<)),,$(error apploader has wrong entry point, must be 0x$(apploader_entry_point)))

apploader.bin: apploader.elf
	$(OBJCOPY) --strip-all -O binary $< $@

apploader.elf: $(apploader_OBJS)
	$(CC) -nostartfiles -nodefaultlibs -Wl,-Ttext=$(apploader_entry_point) -Wl,-e,al_start -Wl,-T,apploader_ldscript.txt $(apploader_OBJS) -o $@

$(apploader_C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -fno-builtin -c $< -o $@

$(apploader_S_OBJS): %.o: %.S
	$(CC) -c $< -o $@

disassembly: apploader.bin dummy
	$(OBJDUMP) -EB -b binary -m powerpc --adjust-vma=$(apploader_entry_point) -D apploader.bin


clean:
	rm -f \
		*~ \
		apploader.elf \
		$(apploader_S_OBJS) $(apploader_C_OBJS)

dist-clean: clean

dummy:

