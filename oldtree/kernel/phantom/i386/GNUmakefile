
PHANTOM_SRCDIR	= ../..
OBJDIR		= ../..

prefix		= /usr/local
exec_prefix	= ${prefix}

include $(OBJDIR)/Makeconf

##### End of configuration section #####

TARGET	= libi386.a 

SRCDIRS += $(PHANTOM_SRCDIR)/phantom/i386

#OIBASE=../../../old/kernel

INCDIRS += . .. $(realpath $(PHANTOM_HOME))/include 

include $(PHANTOM_SRCDIR)/GNUmakerules-lib

include $(PHANTOM_SRCDIR)/phantom/i386/GNUmakerules
