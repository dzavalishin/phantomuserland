#CONV=java -cp $(PHANTOM_HOME)/build/jar/plc.jar ru.dz.pbitmap.Main
CONV=perl ../../../build/bin/bin2hex.pl

SRCDIRS = opensans robotomono 

FILES := $(foreach DIR,$(SRCDIRS),$(wildcard $(DIR)/*))

TTF_FILES := $(filter %.ttf,$(FILES))

CFILES += $(patsubst %.ttf,%.ttf.c,$(notdir $(TTF_FILES)))

#vpath $(SRCDIRS)
VPATH = $(SRCDIRS)

all::     $(CFILES)

clean::
	rm -f $(CFILES) defs.h

%.ttf.c: %.ttf
	$(CONV) $< 1 >$@
	mv $@ $@.a
	gawk -f rename.awk -v name=$(patsubst '-','_',$(patsubst %.ttf.c,%, $@ ) ) <$@.a >$@
	-rm $@.a
