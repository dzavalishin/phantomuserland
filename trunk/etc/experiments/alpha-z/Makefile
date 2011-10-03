CFLAGS += -mmmx -msse -msse2 -g -O3 -DV=${V}
LDFLAGS += -g

all: alpha-z

alpha-z: alpha-z.o main.o
%.o: %.c alpha-z.h Makefile
clean:
	rm -f alpha-z *.o
run: alpha-z
	sh -c "time ./alpha-z"
run-all:
	for (( V=0 ; V < 5 ; ++V )) ; do export V ; echo $V ; $(MAKE) clean run ; done
