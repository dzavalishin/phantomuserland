include $(realpath $(PHANTOM_HOME))/config.mk

default::
	-@mkdir -p lib
	@$(MAKE) all

# subdir offsets - turned off for seems to be bad idea :(

SUBDIRS = $(filter-out $(EXCLUDED_SUBDIRS),  libc crypt gl dev libphantom threads newos libtuned libwin libkern libfreetype vm tmalloc user modules apps)


%.MAKE:
	@$(MAKE) -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs
	cd modules ; make install ; cd ..

install: install-subdirs


clean: clean-subdirs
	rm -f lib/*.a lib/crt0.o 

analyse:: analyse-subdirs

