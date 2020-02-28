
CROSS=powerpc-linux-
CC=$(CROSS)gcc
OBJDUMP=$(CROSS)objdump
OBJCOPY=$(CROSS)objcopy

HOSTCC = gcc

CFLAGS := -O2


lib_C_SRCS = lib.c
lib_C_OBJS = $(patsubst %.c, %.o, $(lib_C_SRCS))

lib_SRCS = $(lib_C_SRCS)
lib_OBJS = $(lib_C_OBJS)

misc_S_SRCS = misc.S
misc_S_OBJS = $(patsubst %.S, %.o, $(misc_S_SRCS))

misc_SRCS = $(misc_S_SRCS)
misc_OBJS = $(misc_S_OBJS)


all: $(lib_OBJS) $(misc_OBJS)

$(lib_C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -fno-builtin -c $< -o $@

$(misc_S_OBJS): %.o: %.S
	$(CC) -c $< -o $@

clean:
	rm -f \
		*~ \
		$(lib_OBJS) $(misc_OBJS)

dist-clean: clean

dummy:

