include ../../config.mk

SRCDIRS = $(filter-out $(EXCLUDED_SUBDIRS), ../sys/src/phantom ../sys/src/test ../sys/src/ru.dz ../roots/src/internal ../apps/test-mutex )
#SRCDIRS += $(filter-out $(EXCLUDED_SUBDIRS), src/tetris )
BINDIR = $(PHANTOM_HOME)/plib/bin
TARGET = classes

FILES := $(foreach DIR,$(SRCDIRS),$(wildcard $(DIR)/*))
PHFILES := $(filter %.ph,$(notdir$(FILES)))
#PCFILES := $(addprefix, $(BINDIR)/, $(patsubst %.ph,%.pc,$(notdir $(PHFILES))))
PCFILES := $(patsubst %.ph,%.pc, $(PHFILES))
D_FILES := $(patsubst %.ph,%.d,  $(PHFILES))

EXCLUDED_PCFILES = stub.class.pc stub.object.pc stub.thread.pc

vpath %.ph $(SRCDIRS)
VPATH=$(SRCDIRS)
GPATH=$(SRCDIRS)


%.d: %.ph
	@echo makedep :-- $@
	gawk  -v src=$< -f ../ph_depend.awk $<


depend: $(D_FILES)
#	@echo $(D_FILES)

clean: 
	-rm *.pc *.lst *.lstc classes *.d *.java *.c *.ll

