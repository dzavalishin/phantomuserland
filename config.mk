# $(realpath ) gives good cywgin path in Windows, and, I believe, is 
# harmless in Unix.


# add ARCH=arm to local-config.mk to compile for arm
-include $(realpath $(PHANTOM_HOME))/local-config.mk


# Directory where to put executables - supposed to be in PATH
ifndef INST_BIN
INST_BIN=$(realpath c:\bin\tools)
endif

ifndef TFTP_PATH
TFTP_PATH=$(realpath $(PHANTOM_HOME))/run/tftp
endif

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
BOARD=$(ARCH)_default
endif


# How to compile phantom source
MKBULK=$(realpath $(PHANTOM_HOME))/build/bin/mkbulk

# Cygwin specific
ifeq ($(OSTYPE),cygwin)
PLC=$(realpath $(PHANTOM_HOME))/build/bin/plc.cmd
else
PLC=$(realpath $(PHANTOM_HOME))/build/bin/plc
endif

# MacOS (Darwin) specific
ifeq "$(shell uname)"  "Darwin"
export TARGET_OS_MAC = 1
export CC_DIR   = /usr/local/Cellar/llvm/4.0.1/bin
endif


%.pc: %.ph
	$(PLC) $<


# Where phantom class files are

PCDIR=$(realpath $(PHANTOM_HOME))/plib/bin

vpath %.pc $(PCDIR)

