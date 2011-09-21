BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

SHORT_NAME=tcpdemo

TARGET	= pmod_$(SHORT_NAME)

SRCDIRS += $(BUILD_ROOT)/modules/$(SHORT_NAME)

include $(BUILD_ROOT)/makerules-usr

include $(BUILD_ROOT)/modules/$(SHORT_NAME)/makerules

#INSTALL_DIR=$(TFTP_PATH)/$(TARGET)
INSTALL_DIR=$(realpath $(PHANTOM_HOME))/run/fat/boot/$(TARGET)

analyse::
	#
