#!/bin/sh

QDIR="qemu/0.14.1"
#QDIR="qemu/0.13.0"
QBIN=qemu

QDIR="qemu/2019.02.18"
QBIN=qemu-system-x86_64w.exe

# SOUND=-soundhw sb16

# USB=-usb -usbdevice mouse
# USB=-usb 

# VIO=-drive file=vio.img,if=virtio,format=raw -net nic,model=virtio
# VIO=-drive file=vio.img,if=virtio,format=raw
# VIO=-net nic,model=virtio

Q_PORTS=" -serial file:serial0.log"


# Q_NET= -net nic,model=ne2k_pci -net user -tftp ./tftp
# Q_NET=" -net nic,model=pcnet -net nic,model=rtl8139  -net user -tftp ./tftp"
Q_NET=" -net nic,model=rtl8139 -net user -tftp ./tftp"


# Q_MACHINE=-m 85

Q_DISKS="-boot a -no-fd-bootchk -fda img/grubfloppy.img -hda fat:fat -hdb phantom.img "
#Q_DISKS="-boot a -no-fd-bootchk -fda img/grubfloppy.img  -hdb phantom.img "

# Q_KQ=-enable-kqemu
#Q_KQ="-kernel-kqemu"

# Q_VGA=-vga std
# Q_VGA=-vga cirrus
Q_VGA="-vga vmware"
# -virtioconsole 4

[ -s local.sh ] && {
echo Loading local.sh
. ./local.sh
}

rm serial0.log.old1
mv serial0.log.old serial0.log.old1
mv serial0.log serial0.log.old

QCMD="$QDIR/$QBIN -smp 3 $Q_VGA -s $Q_KQ  $Q_MACHINE $Q_PORTS $Q_DISKS $Q_NET $VIO $USB $SOUND"
#-L $QDIR/bios

echo Run "$QCMD"
$QCMD

exit


# ----- Unused

# SCSI=-drive file=scsi.img,if=scsi,unit=0
# Q_PORTS= -parallel file:lpt_01.log  -serial file:serial0.log
#    -net nic,model=rtl8139 -net nic,model=i82559er -net nic,model=pcnet -net nic,model=ne2k_isa
# Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy.img -hda fat12.img -hdb phantom.img 
# Q_DISKS=-boot a -fda img/grubfloppy.img -hda fat12.img -hdb phantom.img 
# Q_MACHINE=-M isapc
# DEBUG=
