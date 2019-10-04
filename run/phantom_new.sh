#!/bin/bash

# Test script for Phantom OS
# Requires QEMU 2.5 or higher

Q_DEBUG="-gdb tcp::1234  "
Q_KVM="--enable-kvm "

# Uncomment this to use EFI
# Q_EFI="-L . --bios OVMF.fd "

Q_BOOT="-boot order=c,menu=on "

# Q_CD="-cdrom img/grub2.img "
# Q_DISK_A="-drive file=fat:rw:fat,format=raw,media=disk "
Q_DISK_A="-drive file=phantom_boot.img,format=raw,media=disk "
Q_DISK_B="-drive file=phantom.img,format=raw,media=disk "

# Q_NET="-net nic,model=pcnet -net nic,model=rtl8139 -net user,tftp=tftp "
# Q_PORTS=" -serial file:serial0.log"
Q_PORTS="-serial stdio -soundhw pcspk "

# VIO="-drive file=vio.img,if=virtio,format=raw -net nic,model=virtio"

qemu-system-i386 $Q_KVM $Q_DEBUG $Q_EFI -display sdl -m 256M $Q_PORTS $Q_BOOT $Q_CD $Q_DISK_A $Q_DISK_B $Q_NET $VIO

exit
