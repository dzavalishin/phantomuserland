#CFLAGS += -mmmx -msse -msse2 -g -O3 -DV=${V}
#LDFLAGS += -g

CFLAGS += -mmmx -msse -msse2 -O5 -DV=${V}
LDFLAGS += 

all: alpha-z

alpha-z: alpha-z.o main.o
%.o: %.c alpha-z.h Makefile
clean:
	rm -f alpha-z *.o
run: alpha-z
	echo ${V}
	sleep 4
	sh -c "time ./alpha-z"
run-all:
	for (( V=0 ; V < 5 ; ++V )) ; do export V ; $(MAKE) clean run ; done
