
DEBUG=1

CROSS=
CC=$(CROSS)gcc
OBJDUMP=$(CROSS)objdump
OBJCOPY=$(CROSS)objcopy

HOSTCC = gcc

CFLAGS := -g


parse_gcm_C_SRCS = parse_gcm.c
parse_gcm_C_OBJS = $(patsubst %.c, %.o, $(parse_gcm_C_SRCS))

parse_gcm_SRCS = $(parse_gcm_C_SRCS)
parse_gcm_OBJS = $(parse_gcm_C_OBJS) ../common/lib.o

all: parse_gcm

parse_gcm: $(parse_gcm_OBJS)
	$(CC) -o $@ $+

$(parse_gcm_C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f \
		*~ \
		parse_gcm $(parse_gcm_C_OBJS)

dist-clean: clean

dummy:

