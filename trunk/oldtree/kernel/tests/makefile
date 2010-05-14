all: amap-test
	amap-test

CFLAGS += -I../phantom -I$(realpath $(PHANTOM_HOME))/include

TEST_O = test.o
LPATH = $(realpath $(PHANTOM_HOME))/phantom/lib
LIBS = $(LPATH)/libphantom_c.a 

amap-test: amap-test.o ../phantom/amap.o $(TEST_O)
	gcc -o $@ $< $(LIBS) $(TEST_O)


