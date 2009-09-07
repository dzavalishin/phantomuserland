include config.mk

OBJS= api.o list.o vertex.o init.o matrix.o texture.o \
      misc.o clear.o light.o clip.o select.o get.o error.o \
      zbuffer.o zline.o zdither.o ztriangle.o \
      zmath.o image_util.o oscontext.o msghandling.o \
      arrays.o specbuf.o memory.o phantomdrvr.o phantom_loop.o texobj.o

INCLUDES = -I$(realpath $(PHANTOM_HOME))/include  -I. 
LIB = libTinyGL.a

all: $(LIB)

$(LIB): $(OBJS)
	rm -f $(LIB)
	ar rcs $(LIB) $(OBJS)
	cp $(LIB) ../lib

clean:
	rm -f *~ *.o *.a *.d

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -ffreestanding -MD -DHAVE_CONFIG_H -DOSKIT_X86 -DOSKIT_X86_PC -DINDIRECT_OSENV -nostdinc -O0 -g -c $*.c

clip.o: zgl.h zfeatures.h
vertex.o: zgl.h zfeatures.h
light.o: zgl.h zfeatures.h
matrix.o: zgl.h zfeatures.h
list.o: zgl.h opinfo.h zfeatures.h
arrays.c: zgl.h zfeatures.h
specbuf.o: zgl.h zfeatures.h
glx.o: zgl.h zfeatures.h
nglx.o: zgl.h zfeatures.h
zline.o: zgl.h zfeatures.h zline.h

ztriangle.o: ztriangle.c ztriangle.h zgl.h zfeatures.h
	$(CC) $(CFLAGS) -Wno-uninitialized $(INCLUDES) -c $*.c

-include *.d
