BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= generate

SRCDIRS += $(BUILD_ROOT)/generate

#INCDIRS += ../kernel

include $(BUILD_ROOT)/makerules

include $(BUILD_ROOT)/offsets/makerules

#generate.o:		generate.c 

#we need this because when building for arm we still need default host's gcc to run executable
#does it mean we're toast on arm, btw?
CC=gcc

generate:	generate.o store.o
	gcc -m32 -o generate generate.o store.o
	./generate

CLEAN_FILES += generate generate.exe FieldOffsets.java field_offsets.h 

analyse:: 
	# none
