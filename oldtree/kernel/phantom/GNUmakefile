
PHANTOM_SRCDIR	= ..
OBJDIR		= ..

prefix		= /usr/local
exec_prefix	= ${prefix}

include $(realpath $(PHANTOM_HOME))/config.mk
include $(OBJDIR)/Makeconf-$(ARCH)

##### End of configuration section #####

#include $(PHANTOM_SRCDIR)/machdep/$(HOST_ARCH)/GNUmakerules
include $(PHANTOM_SRCDIR)/phantom/GNUmakerules
