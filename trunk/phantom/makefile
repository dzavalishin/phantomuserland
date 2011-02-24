default:: all

SUBDIRS = $(filter-out $(EXCLUDED_SUBDIRS), offsets libc gl dev libphantom threads newos user vm modules)


%.MAKE:
	@$(MAKE) -j 6 -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs
	cd modules ; make install ; cd ..

install: install-subdirs


clean: clean-subdirs
	rm -f lib/*.a lib/crt0.o 

analyse:: analyse-subdirs

