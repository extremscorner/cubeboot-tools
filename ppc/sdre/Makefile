
CROSS=powerpc-linux-
CC=$(CROSS)gcc
OBJDUMP=$(CROSS)objdump
OBJCOPY=$(CROSS)objcopy

HOSTCC = gcc

CFLAGS := -O2


sdre_entry_point = 80003100

sdre_C_SRCS = sdre.c control.c
sdre_C_OBJS = $(patsubst %.c, %.o, $(sdre_C_SRCS))

sdre_S_SRCS = crt0.S
sdre_S_OBJS = $(patsubst %.S, %.o, $(sdre_S_SRCS))

sdre_SRCS = $(sdre_C_SRCS) $(sdre_S_SRCS)
sdre_OBJS = $(sdre_S_OBJS) ../common/lib.o ../common/misc.o $(sdre_C_OBJS)


all: sdre.bin


sdre.bin: sdre.elf
	$(OBJCOPY) --strip-all -O binary $< $@

sdre.elf: $(sdre_OBJS)
	$(CC) -nostartfiles -nodefaultlibs -Wl,-Ttext=$(sdre_entry_point) -Wl,-T,sdre_ldscript.txt $(sdre_OBJS) -o $@

$(sdre_C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -fno-builtin -c $< -o $@

$(sdre_S_OBJS): %.o: %.S
	$(CC) -c $< -o $@

disassembly: sdre.bin dummy
	$(OBJDUMP) -EB -b binary -m powerpc --adjust-vma=$(sdre_entry_point) -D sdre.bin


clean:
	rm -f \
		*~ \
		sdre.elf sdre.bin \
		$(sdre_S_OBJS) $(sdre_C_OBJS)

dist-clean: clean

dummy:

