all:: unit plc

unit::
	cd unit; cmake .; make; ./runtests; cd ..


plc::
	$(MAKE) -C plc

clean::
	cd unit; sh clean.sh ; cd ..

