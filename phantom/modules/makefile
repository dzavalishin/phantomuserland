# default to all, or else it makes all twice
#default:: all

include $(realpath $(PHANTOM_HOME))/config.mk

ifeq ($(ARCH),e2k)
EXCLUDED_SUBDIRS=test
endif

# TODO divide in bin and modules
SUBDIRS = $(filter-out $(EXCLUDED_SUBDIRS), tcpdemo test )


%.MAKE:
	@$(MAKE) -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs

install: install-subdirs

clean: clean-subdirs

analyse:: analyse-subdirs

