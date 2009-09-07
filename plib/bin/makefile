MKBULK=mkbulk
PLC=plc.cmd

OFF=internal.object.pc internal.io.tty.pc 

# todo
# ru.dz.phantom.system.iterable.pc ru.dz.phantom.system.iterator.pc \


#test.util.pc 

classes: internal.int.pc internal.io.tty.pc internal.object.pc internal.string.pc \
ru.dz.phantom.system.boot.pc ru.dz.phantom.system.class_loader.pc \
ru.dz.phantom.system.regression_tests.pc ru.dz.phantom.system.runnable.pc \
ru.dz.phantom.system.thread_test.pc phantom.util.map.pc \
ru.dz.windows.root.pc ru.dz.phantom.backgrounds.pc \
phantom.osimpl.pc phantom.os.pc phantom.os.time.pc ru.dz.phantom.tetris.*.pc
	$(MKBULK) classes $(filter-out $(OFF) ,$^)
	cp classes g:/tftp/classes

%.pc: ../src/%.ph
	$(PLC) $<


clean: 
	rm *.pc *.lst *.lstc classes
