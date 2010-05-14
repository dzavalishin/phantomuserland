
PHANTOM_SRCDIR	= ../..
OBJDIR		= ../..

prefix		= /usr/local
exec_prefix	= ${prefix}

include $(OBJDIR)/Makeconf

##### End of configuration section #####

TARGET	= libthreads.a test

SRCDIRS += $(PHANTOM_SRCDIR)/phantom/threads

INCDIRS += . $(realpath $(PHANTOM_HOME))/include  

EXCLUDED_OBJFILES=main.o

include $(PHANTOM_SRCDIR)/GNUmakerules-lib

include $(PHANTOM_SRCDIR)/phantom/threads/GNUmakerules
