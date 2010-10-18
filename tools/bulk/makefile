	#gcc -I$(realpath $(PHANTOM_HOME))/include -g -o $@ $^

mkbulk.exe: mkbulk.o pvm_specific.o
	gcc -g -o $@ $^
	cp $@ ../../build/bin

pvm_specific.o: pvm_specific.c
	gcc -c -I$(realpath $(PHANTOM_HOME))/include -g -o $@ $^
        