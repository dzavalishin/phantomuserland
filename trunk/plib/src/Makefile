all: classes

vpath %.pc ../bin


# read stub.object.ph to understand
roots::
	$(PLC) stub.object.ph
	$(PLC) stub.class.ph
	$(PLC) stub.thread.ph
	$(PLC) internal.string.ph
	$(PLC) internal.int.ph
	$(PLC) internal.class.ph
	$(PLC) internal.object.ph
	$(PLC) internal.string.ph
	$(PLC) internal.int.ph
	$(PLC) internal.class.ph

phantom.environment.pc: phantom.util.map.pc

internal.object.pc: internal.object.ph
	make roots
	make classes
	rm ../bin/internal.thread.pc
	make classes

classes: \
     internal.object.pc internal.int.pc internal.string.pc \
     internal.binary.pc internal.class.pc internal.io.tty.pc \
     internal.bitmap.pc internal.mutex.pc phantom.user.pc phantom.util.map.pc \
     ru.dz.phantom.backgrounds.pc ru.dz.phantom.resources.pc \
     phantom.os.time.pc \
     internal.world.pc phantom.osimpl.pc phantom.os.pc \
     phantom.environment.pc \
     internal.thread.pc ru.dz.phantom.system.thread_test.pc \
     internal.connection.pc \
     ru.dz.phantom.system.shell_callback.pc ru.dz.phantom.system.shell.pc



PLC=$(realpath $(PHANTOM_HOME))/build/bin/plc.cmd
BOOTCLASSES=$(realpath $(PHANTOM_HOME))/run/fat/class

%.pc: %.ph
	@echo -:- $< - $@
	@$(PLC) $<
	cp ../bin/$@ $(BOOTCLASSES)

#-include ../bin/*.d

clean::
	-rm ../bin/*.pc
	-rm *.ph~

