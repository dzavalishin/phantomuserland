default::
	@echo make all or make install

install::
	cd build ; make install

# 	echo $(realpath $(PHANTOM_HOME))

all::
	cd plib; make all; cd ..
	cd apps/tetris/tetris; make all; cd ../..
	cd phantom; make all; cd ..
	cd oldtree/kernel; make all; cd ../..

clean::
	cd phantom; make clean; cd ..
	cd oldtree/kernel; make clean; cd ../..
	cd plib; make clean; cd ..

