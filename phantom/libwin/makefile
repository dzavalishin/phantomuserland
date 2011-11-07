BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= libwin.a

SRCDIRS += $(BUILD_ROOT)/libwin $(BUILD_ROOT)/libwin/$(ARCH) $(BUILD_ROOT)/libwin/resources


#ifeq ($(ARCH),ia32)
#PHANTOM_CFLAGS  += -mmmx -msse -msse2 
#endif



include $(BUILD_ROOT)/makerules-lib
include $(BUILD_ROOT)/libwin/makerules
