#!/bin/bash

# Test modern script for Phantom OS with grub2
# Requires QEMU 2.5 or higher

Q_BOOT="-boot order=d,menu=on "
# Q_FLOPPY="-drive file=img/mygrubfloppy.img,format=raw,index=0,if=floppy "
Q_CD="-cdrom img/grub2.iso "
Q_DISK_A="-drive file=fat:rw:fat,format=raw,media=disk "
Q_DISK_B="-drive file=phantom.img,format=raw,media=disk "
# Q_NET="-net nic,model=pcnet -net nic,model=rtl8139 -net user,tftp=tftp "
# Q_PORTS=" -serial file:serial0.log"
Q_PORTS="-serial stdio "
Q_DEBUG="-gdb tcp::1234 "
# Q_KVM="--enable-kvm"
# VIO="-drive file=vio.img,if=virtio,format=raw -net nic,model=virtio"

# rm serial0.log.old1
# mv serial0.log.old serial0.log.old1
# mv serial0.log serial0.log.old

qemu-system-i386 $Q_KVM $Q_DEBUG -display sdl -m 256M $Q_PORTS $Q_BOOT $Q_FLOPPY $Q_CD $Q_DISK_A $Q_DISK_B $Q_NET $VIO

exit
