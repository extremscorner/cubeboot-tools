
DEBUG=1

CROSS=
CC=$(CROSS)gcc

CFLAGS := -g


bnr2ppm_C_SRCS = bnr2ppm.c
bnr2ppm_C_OBJS = $(patsubst %.c, %.o, $(bnr2ppm_C_SRCS))

bnr2ppm_SRCS = $(bnr2ppm_C_SRCS)
bnr2ppm_OBJS = $(bnr2ppm_C_OBJS) ../common/lib.o

all: bnr2ppm

bnr2ppm: $(bnr2ppm_OBJS)
	$(CC) -o $@ $+

$(bnr2ppm_C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f \
		*~ \
		bnr2ppm $(bnr2ppm_C_OBJS)

dist-clean: clean

dummy:

