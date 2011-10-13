CONV=java -cp $(PHANTOM_HOME)/build/jar/plc.jar ru.dz.pbitmap.Main

SRCDIRS = .
FILES := $(foreach DIR,$(SRCDIRS),$(wildcard $(DIR)/*))
PNGFILES := $(filter %.png,$(FILES))
CFILES += $(patsubst %.png,%.png.c,$(notdir $(PNGFILES)))

all::     $(CFILES)

clean::
	rm -f $(CFILES) defs.h

%.png.c: %.png
	$(CONV) $<
	 gawk -f defs.awk $@ >>defs.h
