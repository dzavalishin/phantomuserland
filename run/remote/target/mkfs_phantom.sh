#!/bin/sh

if [ -z "$1" ]; then
    echo "Make Phantom OS FS"
    echo "mkfs_phantom.sh /dev/disk-to-format"
    exit 1
fi


echo Will format Phantom FS @ $1

dd conv=nocreat conv=notrunc bs=4096 count=1 seek=16 if=phantom.superblock of=$1
