# default to all, or else it makes all twice
#default:: all

# TODO divide in bin and modules
SUBDIRS = $(filter-out $(EXCLUDED_SUBDIRS), tcpdemo test )


%.MAKE:
	@$(MAKE) -j 6 -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs

install: install-subdirs

clean: clean-subdirs

analyse:: analyse-subdirs

