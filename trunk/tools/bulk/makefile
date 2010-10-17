	#gcc -I$(realpath $(PHANTOM_HOME))/include -g -o $@ $^

mkbulk.exe: mkbulk.o pvm_cpecific.o
	gcc -g -o $@ $^
	cp $@ ../../build/bin

pvm_cpecific.o: pvm_cpecific.c
	gcc -c -I$(realpath $(PHANTOM_HOME))/include -g -o $@ $^
        