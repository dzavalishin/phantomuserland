include ../../config.mk

SRCDIRS = $(filter-out $(EXCLUDED_SUBDIRS), src/phantom src/ru.dz src/tetris src/test ../roots/src/internal )
BINDIR=../bin
TARGET = classes

FILES := $(foreach DIR,$(SRCDIRS),$(wildcard $(DIR)/*))
PHFILES := $(filter %.ph,$(FILES))
PCFILES := $(patsubst %.ph,%.pc,$(notdir $(PHFILES)))

EXCLUDED_PCFILES = stub.class.pc stub.object.pc stub.thread.pc

all: install

install: classes classes.ar
	cp classes ../bin
	cp classes $(BOOT_PATH)
	cp classes.ar $(BOOT_PATH)
	cp classes $(TFTP_PATH)


vpath %.ph $(SRCDIRS)
vpath %.pc $(BINDIR)

#@sh -c "$(MKBULK) $(TARGET) $(filter-out $(OFF) ,$^)"
# for some unknown reason mkbulk fails if run from make, but is ok when run from shell - win7

$(TARGET): $(filter-out $(EXCLUDED_PCFILES), $(PCFILES) )
	@echo --- make classes bulk ---
	sh -c "$(MKBULK) $(TARGET) $(filter-out $(OFF) ,$^)"

classes.ar: $(filter-out $(EXCLUDED_PCFILES), $(PCFILES) )
	@echo --- make classes ar ---
	@ar r classes.ar $(filter-out $(OFF) ,$^)


roots: internal.thread.pc internal.bitmap.pc
	$(MAKE) -C ../roots

BOOTCLASSES=$(realpath $(PHANTOM_HOME))/run/fat/class

%.pc: %.ph
	@echo :-- $@
	@$(PLC) $<
	@cp $(BINDIR)/$@ $(BOOTCLASSES)

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

ru.dz.phantom.backgrounds.pc: ../resources/backgrounds/phantom_dz_new_1024_768.ppm

internal.bitmap.pc:  internal.io.tty.pc

internal.bootstrap.pc: phantom.osimpl.pc

phantom.osimpl.pc: phantom.os.time.pc

phantom.os.pc: phantom.os.time.pc
phantom.os.pc: phantom.osimpl.pc
phantom.os.pc: internal.world.pc

internal.window.pc: ru.dz.phantom.handler.pc

phantom.environment.pc: phantom.util.map.pc

ru.dz.phantom.system.boot.pc: ru.dz.phantom.system.regression_tests.pc
ru.dz.phantom.system.boot.pc: ru.dz.phantom.system.class_loader.pc
ru.dz.phantom.system.boot.pc: ru.dz.windows.root.pc
ru.dz.phantom.system.boot.pc: ru.dz.phantom.system.thread_test.pc
ru.dz.phantom.system.boot.pc: ru.dz.phantom.system.shell.pc

ru.dz.phantom.system.shell.pc: ru.dz.phantom.system.runnable.pc
ru.dz.phantom.system.shell.pc: ru.dz.phantom.system.shell_callback.pc
ru.dz.phantom.system.shell.pc: test.toPhantom.PhantomPrinter.pc

ru.dz.phantom.system.iterator.pc: ru.dz.phantom.system.iterable.pc

ru.dz.phantom.tetris.mainmodule.pc: ru.dz.phantom.tetris.imagesmanager.pc
ru.dz.phantom.tetris.mainmodule.pc: ru.dz.phantom.tetris.figure.pc
ru.dz.phantom.tetris.mainmodule.pc: ru.dz.phantom.tetris.io.pc

ru.dz.phantom.tetris.figure.pc: ru.dz.phantom.tetris.playingfield.pc
ru.dz.phantom.tetris.io.pc: ru.dz.phantom.tetris.utilites.pc

ru.dz.windows.root.pc: internal.io.tty.pc

ru.dz.windows.root.pc: internal.bitmap.pc

ru.dz.phantom.system.shell_callback.pc: internal.window.pc
ru.dz.phantom.system.shell_callback.pc: internal.connection.pc

ru.dz.phantom.system.shell.pc: internal.mutex.pc

ru.dz.phantom.system.boot.pc: internal.binary.pc

internal.thread.pc: phantom.osimpl.pc
internal.thread.pc: phantom.user.pc

ru.dz.phantom.system.shell_callback.pc: ru.dz.phantom.system.runnable.pc

phantom.util.hashmap.pc: phantom.util.hashpair.pc


ru.dz.demo.start.pc: ru.dz.demo.weather.pc ru.dz.demo.chart.pc
ru.dz.phantom.system.shell.pc: ru.dz.demo.start.pc

