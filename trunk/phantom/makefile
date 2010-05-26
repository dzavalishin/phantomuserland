default:: all

SUBDIRS = $(filter-out $(EXCLUDED_SUBDIRS), libc gl vm user dev libphantom )


%.MAKE:
	@$(MAKE) -j 6 -C $(dir $@) $(basename $(notdir $@))
%-subdirs: $(addsuffix /%.MAKE,$(SUBDIRS)) ;

# By default, make any target by descending into subdirectories.
%: %-subdirs ;

all: all-subdirs

clean: clean-subdirs
	rm -f lib/*.a


#all:
#	cd libc; make -j1; cd ..
#	cd gl; make -j1; cd ..
#	cd vm; make -j1; cd ..

#clean:
#	cd libc; make -j1 clean; cd ..
#	cd gl; make -j1 clean; cd ..
#	cd vm; make -j1 clean; cd ..


