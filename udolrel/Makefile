
DEBUG=1

CROSS=
CC=$(CROSS)gcc
OBJDUMP=$(CROSS)objdump
OBJCOPY=$(CROSS)objcopy

HOSTCC = gcc

CFLAGS := -g


udolrel_C_SRCS = udolrel.c
udolrel_C_OBJS = $(patsubst %.c, %.o, $(udolrel_C_SRCS))

udolrel_SRCS = $(udolrel_C_SRCS)
udolrel_OBJS = $(udolrel_C_OBJS) ../common/lib.o

all: udolrel

udolrel: $(udolrel_OBJS) ../ppc/sdre/sdre.bin
	$(CC) -o $@ $(udolrel_OBJS)

$(udolrel_C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f \
		*~ \
		udolrel \ $(udolrel_C_OBJS)

dist-clean: clean

dummy:

