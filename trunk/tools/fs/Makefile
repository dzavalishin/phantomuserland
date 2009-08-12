
all: pfsextract.exe pfsformat.exe

DEST=../../build/bin

PHINC=$(realpath $(PHANTOM_HOME))/include

#CFLAGS += -I. -I- -I$(PHINC)
#CPPFLAGS += -I. -I- -I$(PHINC)

CFLAGS += -idirafter $(PHINC)
CPPFLAGS += -idirafter $(PHINC)


clean:
	rm *.o *.exe

pfsextract.exe: fsio.o pfsextract.o
	g++ -o $@ $(filter-out %.a,$^)
	cp $@ $(DEST)

pfsformat.exe: pfsformat.o disk_struct.o
	g++ -o $@ $(filter-out %.a,$^)
	cp $@ $(DEST)

#disk_struct.c: ../phantom/disk_struct.c 
#	cp $^ .
