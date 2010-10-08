BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= libphantom_c.a 

SRCDIRS += $(BUILD_ROOT)/libc $(BUILD_ROOT)/libc/$(ARCH) 
# SRCDIRS += $(BUILD_ROOT)/libc/resolv

#INCDIRS += /usr/include/w32api/

include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/libc/makerules
