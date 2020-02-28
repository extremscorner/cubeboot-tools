
SUBDIRS = common apploader sdre

all:
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && make); \
	done; 

clean:
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && make clean); \
	done; 

dist-clean: clean
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && make dist-clean); \
	done;

dummy:

