BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= libtuned.a

SRCDIRS += $(BUILD_ROOT)/libtuned $(BUILD_ROOT)/libtuned/$(ARCH) 


#ifeq ($(ARCH),ia32)
PHANTOM_CFLAGS  += -mmmx -msse -msse2 
#endif



include $(BUILD_ROOT)/makerules-lib
include $(BUILD_ROOT)/libkern/makerules
