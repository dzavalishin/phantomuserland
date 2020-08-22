BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= libphantom_vm.a

SRCDIRS += $(BUILD_ROOT)/vm 
# $(BUILD_ROOT)/vm/cont
#SRCDIRS += $(PHANTOM_HOME)/plib/resources/buttons
SRCDIRS += $(BUILD_ROOT)/vm/resources
SRCDIRS += $(BUILD_ROOT)/vm/sys
SRCDIRS += $(BUILD_ROOT)/vm/video 

INCDIRS += /usr/include/w32api/

EXCLUDED_OBJFILES=pvm_main.o win_screen.o win_hal.o win_bulk.o nonstandalone.o x11_screen.o x11_display.o win_hal_win.o win_screen_win.o headless_screen.o unix_hal.o unix_hal_unix.o 

# Uncomment to enable tracing
# PHANTOM_CFLAGS += -finstrument-functions

include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/vm/makerules
