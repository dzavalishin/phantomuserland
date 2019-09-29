RESDIR=$(realpath $(PHANTOM_HOME)/plib/resources/buttons)
TTF_DIR=$(realpath $(PHANTOM_HOME)/plib/resources/ttfonts)
#CONV=java -cp $(PHANTOM_HOME)/build/jar/plc.jar ru.dz.pbitmap.Main

#vpath %.png $(RESDIR)

#%.png.c: %.png
#	$(CONV) $<

all::  #close.png.c pin.png.c close_pressed.png.c
	cp $(RESDIR)/*.c .
	cp $(TTF_DIR)/*.c .
