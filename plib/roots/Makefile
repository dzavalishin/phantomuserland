include ../../config.mk

SRCDIRS = $(filter-out $(EXCLUDED_SUBDIRS), src src/internal )
BINDIR=../bin

all: roots 

vpath %.ph $(SRCDIRS)
vpath %.pc $(BINDIR)


# read stub.object.ph to understand
roots:
	@echo --- make roots ---
	@$(PLC) src/stub.object.ph
	@$(PLC) src/stub.class.ph
	@$(PLC) src/stub.thread.ph
	@$(PLC) src/stub.int.ph
	@$(PLC) src/stub.long.ph
	@$(PLC) src/internal/internal.string.ph
	@$(PLC) src/internal/internal.int.ph
	@$(PLC) src/internal/internal.long.ph
	@$(PLC) src/internal/internal.class.ph
	@$(PLC) src/internal/internal.object.ph
	@$(PLC) src/internal/internal.string.ph
	@$(PLC) src/internal/internal.int.ph
	@$(PLC) src/internal/internal.float.ph
	@$(PLC) src/internal/internal.double.ph
	@$(PLC) src/internal/internal.class.ph
	@$(PLC) src/internal/internal.directory.ph
	@$(PLC) src/internal/internal.io.tty.ph
	@$(PLC) src/internal/internal.tcp.ph

%.pc: %.ph
	@echo -:- $< - $@
	@$(PLC) $<
	cp $(BINDIR)/$@ $(BOOTCLASSES)


clean::
	-rm $(BINDIR)/*.pc
	-rm $(BINDIR)/*.lst
	-rm $(BINDIR)/*.lstc
#	-rm *.ph~

# auto deps

-include $(BINDIR)/*.d

# manual deps

