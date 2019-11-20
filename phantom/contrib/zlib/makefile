BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET  = libzlib.a

SRCDIRS += $(BUILD_ROOT)/contrib/zlib

INCDIRS += $(BUILD_ROOT)/contrib/zlib

#PHANTOM_CFLAGS += -include $(realpath $(PHANTOM_HOME))/include/compat/nuttx/config.h

include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/contrib/zlib/makerules
