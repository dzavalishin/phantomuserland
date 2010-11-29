BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= generate

SRCDIRS += $(BUILD_ROOT)/generate

#INCDIRS += ../kernel

include $(BUILD_ROOT)/makerules

include $(BUILD_ROOT)/offsets/makerules

#generate.o:		generate.c 

generate:	generate.o store.o
	gcc -m32 -o generate generate.o store.o
	./generate

CLEAN_FILES += generate generate.exe FieldOffsets.java field_offsets.h 
