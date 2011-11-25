BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= libphantom_vm.a

SRCDIRS += $(BUILD_ROOT)/vm $(BUILD_ROOT)/vm/video
# $(BUILD_ROOT)/vm/cont
#SRCDIRS += $(PHANTOM_HOME)/plib/resources/buttons
SRCDIRS += $(BUILD_ROOT)/vm/resources

INCDIRS += /usr/include/w32api/

EXCLUDED_OBJFILES=pvm_main.o win_screen.o win_hal.o win_bulk.o nonstandalone.o


include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/vm/makerules
