BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= generate

SRCDIRS += $(BUILD_ROOT)/generate

#INCDIRS += ../kernel

include $(BUILD_ROOT)/makerules

include $(BUILD_ROOT)/offsets/makerules

#generate.o:		generate.c 

generate:	generate.o
	gcc -o generate generate.o
	./generate
