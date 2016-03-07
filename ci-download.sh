#!/bin/sh
#
# Not tested
#
wget http://wiki.qemu-project.org/download/qemu-1.0.1.tar.gz
mkdir test/qemu
cd test/qemu
tar xf ../../qemu-1.0.1.tar.gz
cd qemu-1.0.1
sh ./configure
make
