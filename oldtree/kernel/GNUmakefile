
#PHANTOM_SRCDIR	= .
PHANTOM_SRCDIR	= $(realpath $(PHANTOM_HOME))/oldtree/kernel
OBJDIR		= .

prefix		= /usr/local
exec_prefix	= ${prefix}

include $(realpath $(PHANTOM_HOME))/config.mk
include $(OBJDIR)/Makeconf-$(ARCH)


SUBDIRS		= $(filter-out $(PHANTOM_EXCLUDED_SUBDIRS), phantom/i386 phantom )



# Get rid of a bunch of nasty built-in implicit rules.
.SUFFIXES:

# How to recursively descend into subdirectories to make general targets such as `all'.
%.MAKE:
	@$(MAKE) -j 1 -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs

clean: clean-subdirs
###	rm -rf lib/*.a


# This target "prepares" the tree by building any machine-generated
# source files, creating links into the source tree, etc.
# (Used by Knit)
.PHONY: prepare
prepare:: prepare-subdirs

distclean: distclean-subdirs


# For testing
check:
	$(MAKE) -C testsuite $@
testsum:
	@$(MAKE) --no-print-directory -C testsuite $@


# Make directories that have to happen first dependencies of all others,
# and directories that have to happen lest dependent on all others.
# Thus parallelism (-j) will not barf.
FIRST_SUBDIRS = 
LAST_SUBDIRS = boot/% phantom/%

$(patsubst %,%/all.MAKE,$(filter-out $(FIRST_SUBDIRS),$(SUBDIRS))): $(FIRST_SUBDIRS:%=%/all.MAKE)
$(patsubst %,%/all.MAKE,$(filter $(LAST_SUBDIRS),$(SUBDIRS))): $(patsubst %,%/all.MAKE,$(filter-out $(LAST_SUBDIRS),$(SUBDIRS)))


debug:
	@echo SUBDIRS = $(SUBDIRS)
