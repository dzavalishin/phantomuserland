BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET  = libvnc.a

SRCDIRS += $(BUILD_ROOT)/contrib/vnc

INCDIRS += $(BUILD_ROOT)/contrib/vnc

#PHANTOM_CFLAGS += -DHAVE_CONFIG_H
#PHANTOM_CFLAGS += -DHAVE_CONFIG_H -DOSKIT_X86 -DOSKIT_X86_PC -DINDIRECT_OSENV
PHANTOM_CFLAGS += -include $(realpath $(PHANTOM_HOME))/include/compat/nuttx/config.h

include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/contrib/vnc/makerules
