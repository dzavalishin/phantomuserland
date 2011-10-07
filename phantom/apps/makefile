#default:: all

# amp mpeg ping 
SUBDIRS = $(filter-out $(EXCLUDED_SUBDIRS), cat dmesg echo grep kill ln ls mkdir mode more rm sh sntp stats tcptest tetris top wavplay wc)


%.MAKE:
	@$(MAKE) -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs

install: install-subdirs

clean: clean-subdirs

analyse:: analyse-subdirs

