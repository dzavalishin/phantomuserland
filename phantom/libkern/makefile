include ../../config.mk

BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= libkern.a

SRCDIRS += $(BUILD_ROOT)/libkern $(BUILD_ROOT)/libkern/snmp $(BUILD_ROOT)/libkern/$(ARCH)
#SRCDIRS += $(BUILD_ROOT)/libkern/net

ifeq ($(ARCH),ia32)
SRCDIRS += $(BUILD_ROOT)/libkern/acpi
endif

include $(BUILD_ROOT)/makerules-lib
include $(BUILD_ROOT)/libkern/makerules

