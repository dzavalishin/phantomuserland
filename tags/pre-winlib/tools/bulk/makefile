include ../../config.mk

ifeq ($(OSTYPE),cygwin)
MKBULK=mkbulk.exe
else
MKBULK=mkbulk
endif

$(MKBULK): mkbulk.o pvm_specific.o
	gcc -g -o $@ $^
	cp $@ ../../build/bin

pvm_specific.o: pvm_specific.c
	gcc -c -I$(realpath $(PHANTOM_HOME))/include -I$(realpath $(PHANTOM_HOME))/include/${ARCH} -g -o $@ $^

clean:
	rm -f $(MKBULK) *.o
