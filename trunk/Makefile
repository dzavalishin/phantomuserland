default::
	@echo make all or make install

install::
	cd build ; make install

# 	echo $(realpath $(PHANTOM_HOME))


classes::
	# classes
	cd plib; make all; cd ..
	cd apps/tetris/tetris; make all; cd ../..
	cd plib; make all; cd ..

classes-clean::
	# classes
	cd plib; make clean; cd ..


all::
	# kernel
	cd phantom; make all; cd ..
	cd oldtree/kernel; make all; cd ../..

clean::
	# kernel
	cd phantom; make clean; cd ..
	cd oldtree/kernel; make clean; cd ../..
	-rm *.E all_sources

analyse::
	cd phantom; make analyse; cd ..
	cd oldtree/kernel; make analyse; cd ../..
	#cat *.E >all_sources
	#rm *.E
	splint +gnuextensions +nolib +boolint *.E >ana
