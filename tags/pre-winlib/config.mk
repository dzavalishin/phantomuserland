# $(realpath ) gives good cywgin path in Windows, and, I believe, is 
# harmless in Unix.


# add ARCH=arm to local-config.mk to compile for arm
-include $(realpath $(PHANTOM_HOME))/local-config.mk


# Directory where to put executables - supposed to be in PATH
INST_BIN=$(realpath c:\bin\tools)

TFTP_PATH=$(realpath $(PHANTOM_HOME))/run/tftp
BOOT_PATH=$(realpath $(PHANTOM_HOME))/run/fat/boot

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

ifndef ARCH
ARCH=ia32
endif

ifndef BOARD
BOARD=$(ARCH)-default
endif


# How to compile phantom source

ifeq ($(OSTYPE),cygwin)
PLC=$(realpath $(PHANTOM_HOME))/build/bin/plc.cmd
MKBULK=$(realpath $(PHANTOM_HOME))/build/bin/mkbulk.exe
else
PLC=$(realpath $(PHANTOM_HOME))/build/bin/plc.sh
MKBULK=$(realpath $(PHANTOM_HOME))/build/bin/mkbulk
endif

%.pc: %.ph
	$(PLC) $<


# Where phantom class files are

PCDIR=$(realpath $(PHANTOM_HOME))/plib/bin

vpath %.pc $(PCDIR)

