BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= libwin.a

SRCDIRS += $(BUILD_ROOT)/libwin $(BUILD_ROOT)/libwin/$(ARCH) $(BUILD_ROOT)/libwin/resources $(BUILD_ROOT)/libwin/fonts $(BUILD_ROOT)/libwin/mice


#ifeq ($(ARCH),ia32)
#PHANTOM_CFLAGS  += -mmmx -msse -msse2 
#endif

# Uncomment to enable tracing
#PHANTOM_CFLAGS  += -finstrument-functions


include $(BUILD_ROOT)/makerules-lib
include $(BUILD_ROOT)/libwin/makerules
