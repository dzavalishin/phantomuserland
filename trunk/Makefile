default::
	@echo make all or make install

install::
	cd build ; make install

# 	echo $(realpath $(PHANTOM_HOME))

all::
	cd phantom; make all; cd ..
	cd oldtree/kernel; make all; cd ../..
	cd plib; make all; cd ..

clean::
	cd phantom; make clean; cd ..
	cd oldtree/kernel; make clean; cd ../..
	cd plib; make clean; cd ..

