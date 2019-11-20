BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET  = libpng.a

SRCDIRS += $(BUILD_ROOT)/contrib/png

INCDIRS += $(BUILD_ROOT)/contrib/png

PHANTOM_CFLAGS += -include $(realpath $(PHANTOM_HOME))/include/sys/libkern.h
PHANTOM_CFLAGS += -DPNG_FLOATING_POINT_SUPPORTED -DPNG_NO_CONSOLE_IO -DPNG_NO_STDIO
# -DPNG_ABORT=panic("libpng")

include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/contrib/png/makerules
