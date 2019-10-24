#default:: all

include $(realpath $(PHANTOM_HOME))/config.mk


#FILES := $(foreach DIR,$(SRCDIRS),$(wildcard $(DIR)/*))
#PCFILES := $(patsubst %.ph,%.pc,$(notdir $(PHFILES)))


SUBDIRS = $(filter-out $(EXCLUDED_SUBDIRS), test-mutex)


%.MAKE:
	@$(MAKE) -C $(dir $@) $(basename $(notdir $@))

%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs

clean: clean-subdirs


