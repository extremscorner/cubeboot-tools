
DEBUG=1

CROSS=
CC=$(CROSS)gcc
OBJDUMP=$(CROSS)objdump
OBJCOPY=$(CROSS)objcopy

HOSTCC = gcc

CFLAGS := -g


ppm2bnr_C_SRCS = ppm2bnr.c
ppm2bnr_C_OBJS = $(patsubst %.c, %.o, $(ppm2bnr_C_SRCS))

ppm2bnr_SRCS = $(ppm2bnr_C_SRCS)
ppm2bnr_OBJS = $(ppm2bnr_C_OBJS) ../common/lib.o

all: ppm2bnr

ppm2bnr: $(ppm2bnr_OBJS)
	$(CC) -o $@ $+ -lnetpbm -lm

$(ppm2bnr_C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f \
		*~ \
		ppm2bnr $(ppm2bnr_C_OBJS)

dist-clean: clean

dummy:

