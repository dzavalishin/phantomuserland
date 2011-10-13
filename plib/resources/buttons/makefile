SRCS=close.png.c pin.png.c close_pressed.png.c
all::  $(SRCS)

clean::
	rm -f $(SRCS)

CONV=java -cp $(PHANTOM_HOME)/build/jar/plc.jar ru.dz.pbitmap.Main

#close.png.c: close.png
#	$(CONV) $<

#pin.png.c: pin.png
#	$(CONV) $<

%.png.c: %.png
	$(CONV) $<
