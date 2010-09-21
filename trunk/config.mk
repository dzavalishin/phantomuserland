# $(realpath ) gives good cywgin path in Windows, and, I believe, is 
# harmless in Unix.

# Directory where to put executables - supposed to be in PATH
INST_BIN=$(realpath c:\bin\tools)

ifndef OSTYPE
ifeq ($(OS),Windows_NT)
OSTYPE=cygwin
endif
endif

#all:
#ifeq ($(OSTYPE),cygwin)
#	echo Cygwin
#else
#ifeq ($(OSTYPE),linux-gnu)
#	echo Linux
#else
#	echo Unknown
#endif
#endif

ARCH=ia32

# How to compile phantom source

PLC=$(realpath $(PHANTOM_HOME))/build/bin/plc.cmd

%.pc: %.ph
	$(PLC) $<


# Where phantom class files are

PCDIR=$(realpath $(PHANTOM_HOME))/plib/bin

vpath %.pc $(PCDIR)

