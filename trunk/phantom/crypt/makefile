BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= libcrypt.a

SRCDIRS += $(BUILD_ROOT)/crypt $(BUILD_ROOT)/crypt/$(ARCH) 
#SRCDIRS += $(BUILD_ROOT)/libc/net
# SRCDIRS += $(BUILD_ROOT)/libc/resolv

include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/crypt/makerules
