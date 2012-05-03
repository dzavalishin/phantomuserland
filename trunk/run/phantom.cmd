@echo off
rem http://dietpc.org/windows/qemu/
rem SET QDIR=qemu\0.13.0
rem SET QDIR=qemu\0.14.1
SET QDIR=qemu\0.15.1

set QEMU_AUDIO_DRV=dsound
set QEMU_AUDIO_DRV=sdl
rem set QEMU_AUDIO_DRV=fmod
rem set SOUND=-soundhw sb16,es1370 -device intel-hda -device hda-duplex

rem SET USB=-device pci-ohci,id=ohci -device usb-mouse,bus=ohci.0
rem SET USB=-device pci-ohci,id=ohci -device usb-mouse,bus=/i440FX-pcihost/pci.0/ohci/ohci.0
rem SET USB=-device pci-ohci -usbdevice mouse
rem SET USB=-usb -device pci-ohci -usbdevice mouse 
rem SET USB=-usb -usbdevice serial::tcp:ya.ru:80
rem SET USB=-usb -usbdevice keyboard

rem SET VIO=-drive file=vio.img,if=virtio,format=raw -net nic,model=virtio
rem SET VIO=-drive file=vio.img,if=virtio,format=raw
rem SET VIO=-net nic,model=virtio

rem SET Q_REDIR=-redir udp:123::123
SET Q_REDIR=-redir udp:8023::23

SET Q_PORTS= -serial file:serial0.log

SET Q_AHCI=-drive id=disk,file=ahci.img,if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0 

rem SET Q_NET= -net nic,model=ne2k_pci -net user -tftp ./tftp
rem SET Q_NET= -net nic,model=pcnet -net nic,model=rtl8139  -net user -tftp ./tftp
SET Q_NET= -net nic,model=rtl8139  -net user,tftp=./tftp -tftp ./tftp
rem SET Q_NET= -net nic,model=pcnet  -net user -tftp ./tftp


SET Q_MACHINE=-m 256

SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy-hd0.img -hda fat:fat -hdb phantom.img 
rem -fdb kolibri.iso
rem SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy-hd0.img -fdb e2.img -hda fat:fat -hdb phantom.img 
rem SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy-hd0.img -hda fat:fat -hdb phantom.img 

rem SET Q_KQ=-enable-kqemu
rem SET Q_KQ=-enable-kqemu -kernel-kqemu

rem SET Q_VGA=-vga std
rem SET Q_VGA=-vga cirrus
SET Q_VGA=-vga vmware
rem SET Q_VGA=-device cirrus-vga
rem -virtioconsole 4

del serial0.log.old1
ren serial0.log.old serial0.log.old1
ren serial0.log serial0.log.old
%QDIR%\qemu -net dump,file=net.dmp -smp 3 %Q_VGA% -gdb tcp::1234,nowait,nodelay,server,ipv4 %Q_KQ% -L %QDIR%\bios %Q_MACHINE% %Q_PORTS% %Q_DISKS% %Q_NET% %VIO% %USB% %SOUND% %Q_AHCI% %Q_REDIR%

grep KERNEL.TEST serial0.log
grep USERMODE.TEST serial0.log

exit


rem ----- Unused

rem SET SCSI=-drive file=scsi.img,if=scsi,unit=0
rem SET Q_PORTS= -parallel file:lpt_01.log  -serial file:serial0.log
rem    -net nic,model=rtl8139 -net nic,model=i82559er -net nic,model=pcnet -net nic,model=ne2k_isa
rem SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy.img -hda fat12.img -hdb phantom.img 
rem SET Q_DISKS=-boot a -fda img/grubfloppy.img -hda fat12.img -hdb phantom.img 
rem SET Q_MACHINE=-M isapc
rem SET DEBUG=
