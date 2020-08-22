#!/bin/sh
rm phantom.img
touch phantom.img
# 112 mb
#dd bs=1048576 seek=0 count=112 if=/dev/zero of=phantom.img
# 500 mb
dd bs=1048576 seek=0 count=500 if=/dev/zero of=phantom.img
dd conv=nocreat conv=notrunc bs=4096 count=1 seek=16 if=img/phantom.superblock of=phantom.img
# dd if=/dev/zero of=snapcopy.img bs=4096 skip=1 count=1024
dd if=/dev/zero of=vio.img bs=4096 skip=1 count=1024
