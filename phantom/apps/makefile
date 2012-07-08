#default:: all

# amp mpeg ping 
SUBDIRS = $(filter-out $(EXCLUDED_SUBDIRS), cat cp dmesg echo grep kill ln ls mkdir mode more pwd rm sh sntp stats tcp_echo tcptest tetris top touch udp_echo wavplay wc)


%.MAKE:
	@$(MAKE) -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs

install: install-subdirs

clean: clean-subdirs

analyse:: analyse-subdirs

