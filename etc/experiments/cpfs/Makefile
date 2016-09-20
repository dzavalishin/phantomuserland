#ARCHFLAGS = -m32
CFLAGS +=-g -Wall -Wextra $(ARCHFLAGS) -std=c99
LDFLAGS += -g -lpthread $(ARCHFLAGS)

cpfs: main.o os.o \
	cpfs.o cpfs_io.o cpfs_init.o cpfs_sb.o cpfs_blkio.o cpfs_inode.o cpfs_ino_io.o \
        cpfs_dir.o cpfs_alloc.o cpfs_buffer.o cpfs_fdmap.o cpfs_fsck.o cpfs_path.o cpfs_unlink.o \
        cpfs_indirect.o \
        cpfs_test.o cpfs_test_mp.o \
        arinc_fs.o


clean::
	-rm *.o
	-rm *.img

# is this size ok for size we use in mkfs?
image::
	dd if=/dev/zero of=disk.img count=10000 bs=4096
	dd if=/dev/zero of=disk1.img count=12000 bs=4096


test:
	make clean
	make
	make image
	@echo
	@echo
	./cpfs
	@echo
	@echo Finished tests of cpfs

