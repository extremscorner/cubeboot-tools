
DEBUG=1

CROSS=
CC=$(CROSS)gcc

HEXDUMP = hexdump

CFLAGS := -g


mkgbi_C_SRCS = mkgbi.c
mkgbi_C_OBJS = $(patsubst %.c, %.o, $(mkgbi_C_SRCS))

mkgbi_SRCS = $(mkgbi_C_SRCS)
mkgbi_OBJS = $(mkgbi_C_OBJS) ../common/lib.o

all: gbi.hdr

gbi.hdr: mkgbi ../ppc/apploader/apploader.bin
	./mkgbi -a ../ppc/apploader/apploader.bin -b ../icons/opening.bnr > $@

mkgbi: $(mkgbi_OBJS)
	$(CC) -o $@ $+

$(mkgbi_C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

hexdump_gbi: gbi.hdr
	$(HEXDUMP) -C $<

clean:
	rm -f \
		*~ \
		mkgbi \ $(mkgbi_C_OBJS)

dist-clean: clean
	rm -f \
		gbi.hdr

dummy:

