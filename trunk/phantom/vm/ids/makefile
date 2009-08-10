#.autodepend

OUT=opcode_ids.java opcode_ids.h

goal:: $(OUT)

CPP=cpp -P -C
#CPP=cpp32 -q 

clean:
	del $(OUT)

opcode_ids.java: opcode_ids.definitions  inc_txt2java/*
	$(CPP) -Iinc_txt2java opcode_ids.definitions $@
#	rm opcode_ids.java
#	mv opcode_ids.i opcode_ids.java

opcode_ids.h: opcode_ids.definitions  inc_txt2c/*
	$(CPP) -Iinc_txt2c opcode_ids.definitions $@.1 
	sed 's/const unsigned char/#define/' $@.1 > $@.2
	sed 's/=/ /' $@.2 | sed 's/;/ /' > $@
#	sed 's!\/\/.*$!!' $@.3 > $@

#	rm opcode_ids.h
#	mv opcode_ids.i opcode_ids.h



