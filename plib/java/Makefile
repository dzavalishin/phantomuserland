include ../../config.mk

JDK_SRC_LIST=java/lang/StringBuilder.java 
JDK_SRC_LIST1 := addprefix($(PHANTOM_JDK_SRC)/,$(JDK_SRC_LIST))
#JDK_BIN_LIST := $(patsubst %.java,%.class,$(notdir $(JDK_SRC_LIST)))
JDK_BIN_LIST := $(patsubst %.java,bin/%.class,$(JDK_SRC_LIST))

vpath %.java $(PHANTOM_JDK_SRC)
vpath %.class bin

#vpath %.ph $(SRCDIRS)
#vpath %.pc $(BINDIR)

%.class: %.java
	@echo :-- $@
	#@$(PLC) $<
	#@cp $(BINDIR)/$@ $(BOOTCLASSES)

#	@echo -:- $< - $@


all: $(JDK_BIN_LIST)

#SRCDIRS = $(filter-out $(EXCLUDED_SUBDIRS), src/phantom src/ru.dz src/tetris src/test ../roots/src/internal )
#BINDIR=../bin
#TARGET = classes

#FILES := $(foreach DIR,$(SRCDIRS),$(wildcard $(DIR)/*))
#PHFILES := $(filter %.ph,$(FILES))
#PCFILES := $(patsubst %.ph,%.pc,$(notdir $(PHFILES)))

#EXCLUDED_PCFILES = stub.class.pc stub.object.pc stub.thread.pc

#all: install

#install: classes classes.ar
#	cp classes $(TFTP_PATH)
#	cp classes $(BOOT_PATH)
#	cp classes ../bin
#	cp classes.ar $(BOOT_PATH)





clean::
	-rm -r bin/*
#	-rm $(BINDIR)/*.pc
#	-rm $(BINDIR)/*.lst
#	-rm $(BINDIR)/*.lstc
#	-rm $(BINDIR)/*.ll
#	-rm *.ph~

# auto deps

#-include $(BINDIR)/*.d

# manual deps

