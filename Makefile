include config.mk
MAKEFLAGS += --print-directory


default::
	@echo make all or make install
	@echo doing make all by default
	$(MAKE) all

install::
	cd build ; make install

# 	echo $(realpath $(PHANTOM_HOME))


classes::
	# classes
	$(MAKE) -C plib all
	$(MAKE) -C apps/tetris/tetris all
	$(MAKE) -C plib all

classes-clean::
	# classes
	$(MAKE) -C plib clean

up::
	$(MAKE) clean
	svn up
	$(MAKE) all

all::
	# kernel
	$(MAKE) -C phantom
ifeq ($(ARCH),ia32)
	$(MAKE) -C oldtree/kernel/phantom/i386
endif
	$(MAKE) -C oldtree/kernel

clean::
	# kernel
	$(MAKE) -C phantom clean
	$(MAKE) -C oldtree/kernel clean
	-rm -f *.E all_sources

analyse::
	$(MAKE) -C phantom analyse
	$(MAKE) -C oldtree/kernel analyse
	-rm -f libTinyGL.a_sources.E 
	#cat *.E >all_sources
	#rm *.E
	splint +gnuextensions +nolib +boolint *.E >ana

rea::
	splint +gnuextensions +nolib +boolint *.E >ana


run:: all
	cd run; sh ./phantom_clean.sh

boot:: #all
	cp oldtree/run/tftp/* $(HW_BOOT_DEST)

test::
	#cd test
	$(MAKE) -C test
