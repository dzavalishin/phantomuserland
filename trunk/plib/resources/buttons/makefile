all::  close.png.c pin.png.c

CONV=java -cp $(PHANTOM_HOME)/build/jar/plc.jar ru.dz.pbitmap.Main

close.png.c: close.png
	$(CONV) $<

pin.png.c: pin.png
	$(CONV) $<
