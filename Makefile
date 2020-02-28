
disc_directory_tree = disc/
disc_image = disc.iso
bootloader = bootldr.dol

MKISOFS = mkisofs
HEXDUMP = hexdump

SUBDIRS = ppc common ppm2bnr icons mkgbi udolrel
EXTRA_SUBDIRS = parse_gcm bnr2ppm

all:
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && make); \
	done;

iso9660: mkgbi/gbi.hdr 
	$(MKISOFS) -R -J -G mkgbi/gbi.hdr -no-emul-boot -boot-load-seg 0 -b $(bootloader) -o $(disc_image) $(disc_directory_tree)

clean:
	@for subdir in $(SUBDIRS) $(EXTRA_SUBDIRS); do \
		(cd $$subdir && make clean); \
	done; 

dist-clean: clean
	@for subdir in $(SUBDIRS) $(EXTRA_SUBDIRS); do \
		(cd $$subdir && make dist-clean); \
	done;

dummy:

