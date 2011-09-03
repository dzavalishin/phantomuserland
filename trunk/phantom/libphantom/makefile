BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= libphantom.a

#SRCDIRS += $(BUILD_ROOT)/libphantom $(BUILD_ROOT)/libphantom/udis86 
SRCDIRS += $(BUILD_ROOT)/libphantom $(BUILD_ROOT)/libphantom/resources $(BUILD_ROOT)/libphantom/video $(BUILD_ROOT)/libphantom/$(ARCH)

INCDIRS += ../kernel

include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/libphantom/makerules
