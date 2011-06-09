include ../../config.mk

all:: classes

MKBULK=mkbulk
PLC=plc.cmd

OFF=internal.object.pc internal.io.tty.pc 

# todo
# ru.dz.phantom.system.iterable.pc ru.dz.phantom.system.iterator.pc \


#test.util.pc 

classes: internal.int.pc internal.io.tty.pc internal.object.pc internal.string.pc \
ru.dz.phantom.system.boot.pc ru.dz.phantom.system.class_loader.pc \
ru.dz.phantom.system.regression_tests.pc ru.dz.phantom.system.runnable.pc \
ru.dz.phantom.system.thread_test.pc phantom.util.map.pc ru.dz.phantom.system.shell.pc \
ru.dz.windows.root.pc ru.dz.phantom.backgrounds.pc ru.dz.phantom.resources.pc \
phantom.osimpl.pc phantom.os.pc phantom.os.time.pc ru.dz.phantom.tetris.*.pc ru.dz.phantom.tetris.boot.pc \
#ru.dz.phantom.system.test*.pc 
	@echo --- Make class bundle
	@$(MKBULK) classes $(filter-out $(OFF) ,$^)


%.pc: ../src/%.ph
	@echo --- plc $<
	@$(PLC) $<


install: classes
	cp classes $(TFTP_PATH)

clean: 
	-rm *.pc *.lst *.lstc classes *.d
