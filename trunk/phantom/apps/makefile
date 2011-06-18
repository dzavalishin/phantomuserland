#default:: all

# amp mpeg ping 
SUBDIRS = $(filter-out $(EXCLUDED_SUBDIRS), cat echo grep kill ln ls mkdir rm sh sntp tetris wavplay wc )


%.MAKE:
	@$(MAKE) -j 6 -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs

install: install-subdirs

clean: clean-subdirs

analyse:: analyse-subdirs

