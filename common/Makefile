
DEBUG=1

CROSS=
CC=$(CROSS)gcc
OBJDUMP=$(CROSS)objdump
OBJCOPY=$(CROSS)objcopy

HOSTCC = gcc

CFLAGS := -g


lib_C_SRCS = lib.c
lib_C_OBJS = $(patsubst %.c, %.o, $(lib_C_SRCS))

all: $(lib_C_OBJS)

$(lib_C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f \
		*~ \
		$(lib_C_OBJS)

dist-clean: clean

dummy:

