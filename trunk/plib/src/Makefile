all: classes

# read stub.object.ph to understand
roots::
	$(PLC) stub.object.ph
	$(PLC) stub.class.ph
	$(PLC) internal.string.ph
	$(PLC) internal.int.ph
	$(PLC) internal.class.ph
	$(PLC) internal.object.ph
	$(PLC) internal.string.ph
	$(PLC) internal.int.ph
	$(PLC) internal.class.ph

../bin/internal.object.pc: internal.object.ph
	make roots

#../bin/internal.int.pc: roots
#../bin/internal.string.pc: roots

classes: \
     ../bin/internal.object.pc ../bin/internal.int.pc ../bin/internal.string.pc \
     ../bin/internal.binary.pc ../bin/internal.class.pc \
     ../bin/internal.io.tty.pc ../bin/internal.thread.pc \
     ../bin/internal.bitmap.pc \
     ../bin/ru.dz.phantom.backgrounds.pc \
#     ../bin/


PLC=plc.cmd

../bin/%.pc: %.ph
	$(PLC) $<
