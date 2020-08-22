#!/bin/bash

# Test script for Phantom OS
# Requires QEMU 2.5 or higher

#Q_VGA="-vga cirrus"
Q_VGA="-vga vmware -display sdl"
#

Q_DEBUG="-gdb tcp::1234  "
#Q_KVM="--enable-kvm "

# Uncomment this to use EFI
# Q_EFI="-L . --bios OVMF.fd "

#Q_BOOT="-boot order=c,menu=on "

# Q_CD="-cdrom img/grub2.img "
# Q_DISK_A="-drive file=fat:rw:fat,format=raw,media=disk "
#Q_DISK_A="-drive file=phantom_boot.img,format=raw,media=disk "
#Q_DISK_B="-drive file=phantom.img,format=raw,media=disk "

Q_DISK_A="-boot a -no-fd-bootchk -drive file=img/grubfloppy-hd0.img,index=0,if=floppy,format=raw -drive file=fat:rw:fat,format=raw -drive if=ide,index=1,file=phantom.img,format=raw"

# Q_NET="-net nic,model=pcnet -net nic,model=rtl8139 -net user,tftp=tftp "
Q_PORTS=" -serial file:serial0.log -soundhw es1370"
Q_PORTS="-serial stdio -soundhw pcspk "

# VIO="-drive file=vio.img,if=virtio,format=raw -net nic,model=virtio"

#Q_NET=" -net nic,model=ne2k_pci -net user -tftp ./tftp"
Q_NET=" -net nic,model=ne2k_pci -net user"


../../tmp/phantom_run_qemu/2019.02.18/qemu-system-i386 $Q_KVM $Q_DEBUG $Q_EFI -m 256M $Q_PORTS $Q_BOOT $Q_CD $Q_DISK_A $Q_DISK_B $Q_NET $VIO $Q_VGA

exit
