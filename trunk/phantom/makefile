default:: all

SUBDIRS = $(filter-out $(EXCLUDED_SUBDIRS), offsets libc gl vm dev libphantom newos user modules/test )


%.MAKE:
	@$(MAKE) -j 6 -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs

clean: clean-subdirs
	rm -f lib/*.a




