CFLAGS +=-g


cpfs: main.o cpfs.o cpfs_test.o cpfs_io.o cpfs_init.o cpfs_sb.o cpfs_blkio.o cpfs_inode.o cpfs_ino_io.o cpfs_dir.o cpfs_alloc.o


clean::
	-rm *.o

image::
	dd if=/dev/zero of=disk.img count=100000


test:
	make clean
	make
	make image
	@echo
	@echo
	./cpfs
	@echo
	@echo Finished tests of cpfs

