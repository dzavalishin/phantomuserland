include ../../config.mk

SRCDIRS = $(filter-out $(EXCLUDED_SUBDIRS), src/phantom src/ru.dz src/tetris src/test ../roots/src/internal )
BINDIR = $(PHANTOM_HOME)/plib/bin
TARGET = classes

FILES := $(foreach DIR,$(SRCDIRS),$(wildcard $(DIR)/*))
PHFILES := $(filter %.ph,$(FILES))
#PCFILES := $(addprefix, $(BINDIR)/, $(patsubst %.ph,%.pc,$(notdir $(PHFILES))))
PCFILES := $(patsubst %.ph,%.pc,$(notdir $(PHFILES)))

EXCLUDED_PCFILES = stub.class.pc stub.object.pc stub.thread.pc

vpath %.ph $(SRCDIRS)
vpath %.pc $(BINDIR)

all: install

install: classes classes.ar
	cp classes $(BINDIR)
	cp classes $(BOOT_PATH)
	cp classes.ar $(BOOT_PATH)
	cp classes $(TFTP_PATH)


#@sh -c "$(MKBULK) $(TARGET) $(filter-out $(OFF) ,$^)"
# for some unknown reason mkbulk fails if run from make, but is ok when run from shell - win7

$(TARGET): $(addprefix $(BINDIR)/,$(filter-out $(EXCLUDED_PCFILES), $(PCFILES) ))
	@echo --- make classes bulk ---
	@java -jar ${PHANTOM_HOME}/build/jar/mkbulk.jar $@ $(filter-out $(OFF) ,$^)
#	$(MKBULK) $(TARGET) $(filter-out $(OFF) ,$^)

#	sh -c "$(MKBULK) $(TARGET) $(filter-out $(OFF) ,$^)"

classes.ar: $(addprefix $(BINDIR)/,$(filter-out $(EXCLUDED_PCFILES), $(PCFILES) ))
	@echo --- make classes ar ---
	@ar r $@ $(filter-out $(OFF) ,$^)


roots: internal.thread.pc internal.bitmap.pc
	$(MAKE) -C ../roots

BOOTCLASSES=$(realpath $(PHANTOM_HOME))/run/fat/class

$(BINDIR)/%.pc: %.ph
	@echo :-- $@
	@java -jar $(PHANTOM_HOME)/build/jar/plc.jar -o$(BINDIR) -I$(BINDIR) $<
	@cp $@ $(BOOTCLASSES)
#	@$(PLC) $<

#	@echo -:- $< - $@

clean::
	-rm $(BINDIR)/*.pc
	-rm $(BINDIR)/*.lst
	-rm $(BINDIR)/*.lstc
	-rm $(BINDIR)/*.ll
#	-rm *.ph~

# auto deps

-include $(BINDIR)/*.d

# manual deps
$(BINDIR)/internal.bitmap.pc: internal.io.tty.pc
$(BINDIR)/internal.bootstrap.pc: phantom.osimpl.pc
$(BINDIR)/internal.thread.pc: phantom.osimpl.pc phantom.user.pc
$(BINDIR)/internal.window.pc: ru.dz.phantom.handler.pc

$(BINDIR)/phantom.environment.pc: phantom.util.map.pc
$(BINDIR)/phantom.osimpl.pc: phantom.os.time.pc
$(BINDIR)/phantom.os.pc: phantom.os.time.pc phantom.osimpl.pc internal.world.pc
$(BINDIR)/phantom.util.hashmap.pc: phantom.util.hashpair.pc

$(BINDIR)/ru.dz.demo.chart.pc: phantom.os.pc internal.io.tty.pc internal.window.pc internal.connection.pc ru.dz.phantom.system.runnable.pc
$(BINDIR)/ru.dz.demo.start.pc: ru.dz.demo.weather.pc ru.dz.demo.chart.pc
$(BINDIR)/ru.dz.demo.weather.pc: ru.dz.demo.chart.pc internal.bitmap.pc

$(BINDIR)/ru.dz.phantom.backgrounds.pc: ../resources/backgrounds/phantom_dz_new_1024_768.ppm
$(BINDIR)/ru.dz.phantom.system.boot.pc: ru.dz.phantom.system.regression_tests.pc ru.dz.phantom.system.class_loader.pc ru.dz.windows.root.pc ru.dz.phantom.system.thread_test.pc ru.dz.phantom.system.shell.pc internal.binary.pc
$(BINDIR)/ru.dz.phantom.system.iterator.pc: ru.dz.phantom.system.iterable.pc
$(BINDIR)/ru.dz.phantom.system.shell.pc: ru.dz.phantom.system.runnable.pc ru.dz.phantom.system.shell_callback.pc test.toPhantom.PhantomPrinter.pc internal.mutex.pc ru.dz.demo.start.pc
$(BINDIR)/ru.dz.phantom.system.shell_callback.pc: internal.window.pc internal.connection.pc ru.dz.phantom.system.runnable.pc

$(BINDIR)/ru.dz.phantom.tetris.figure.pc: ru.dz.phantom.tetris.playingfield.pc
$(BINDIR)/ru.dz.phantom.tetris.io.pc: ru.dz.phantom.tetris.utilites.pc
$(BINDIR)/ru.dz.phantom.tetris.mainmodule.pc: ru.dz.phantom.tetris.imagesmanager.pc ru.dz.phantom.tetris.figure.pc ru.dz.phantom.tetris.io.pc

$(BINDIR)/ru.dz.windows.root.pc: internal.io.tty.pc internal.bitmap.pc

$(BINDIR)/test.hier.child1.pc: test.hier.intermediate.pc

$(BINDIR)/test.hier.intermediate.pc: test.hier.root.pc

$(BINDIR)/test.suite.pc: test.hier.child1.pc
