BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= libdev.a

SRCDIRS += $(BUILD_ROOT)/dev $(BUILD_ROOT)/dev/ac97 $(BUILD_ROOT)/dev/$(ARCH)
#$(BUILD_ROOT)/dev/ataio

NOT_ANALYSE += ata_driver.c  ataioint.c  ataiopci.c  ataiopio.c  ataioreg.c  ataiosub.c  ataiotmr.c  ataiotrc.c

INCDIRS += ../kernel

include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/dev/makerules
