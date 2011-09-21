BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= pmod_test

SRCDIRS += $(BUILD_ROOT)/modules/test

include $(BUILD_ROOT)/makerules-usr

include $(BUILD_ROOT)/modules/test/makerules

#INSTALL_DIR=$(TFTP_PATH)/$(TARGET)
INSTALL_DIR=$(realpath $(PHANTOM_HOME))/run/fat/boot/$(TARGET)

analyse::
	#
