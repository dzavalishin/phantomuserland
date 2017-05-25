@echo off
rem http://dietpc.org/windows/qemu/
rem http://lassauge.free.fr/qemu/

SET QDIR=qemu\0.15.1
SET QCMD=qemu.exe

rem SET QDIR=qemu\1.2.0
SET QDIR=qemu\1.0.1
SET QCMD=qemu-system-i386.exe

rem SET QDIR=qemu\2.4.1
rem SET QCMD=qemu-system-i386w.exe

rem SET QCMD=qemu-system-x86_64w.exe 

rem SET QDIR=C:\bin\qemu
rem SET QCMD=qemu-system-x86_64.exe

set QEMU_AUDIO_DRV=dsound
set QEMU_AUDIO_DRV=sdl
rem set QEMU_AUDIO_DRV=fmod
rem set SOUND=-soundhw sb16,es1370 -device intel-hda -device hda-duplex
set SOUND=-soundhw sb16,es1370

rem SET USB=-device pci-ohci,id=ohci -device usb-mouse,bus=ohci.0 -device usb-kbd,bus=ohci.0
rem SET USB=-device pci-ohci,id=ohci -device usb-mouse,bus=/i440FX-pcihost/pci.0/ohci/ohci.0
rem SET USB=-device pci-ohci -usbdevice mouse
rem SET USB=-usb -device pci-ohci -usbdevice mouse 
rem SET USB=-usb -usbdevice serial::tcp:ya.ru:80
rem SET USB=-usb -usbdevice keyboard -usbdevice mouse
rem SET USB=-device usb-ehci,id=ehci -usbdevice disk::usb.img

rem SET USB=-usb -usbdevice keyboard
rem SET USB=-usb -usbdevice mouse
rem SET USB=-usb -usbdevice keyboard -usbdevice mouse

rem  -virtioconsole vioc,chardev=vioc -chardev vc,id=vioc 
rem SET VIO= -drive file=vio.img,if=virtio,format=raw -net nic,model=virtio -net user -tftp ./tftp
rem SET VIO=-drive file=vio.img,if=virtio,format=raw 
rem SET VIO=-net nic,model=virtio
rem SET VIO= -device virtio-rng-pci

rem 161 = snmp
rem SET Q_REDIR=-redir udp:123::123
SET Q_REDIR=-redir udp:8023::23 -redir udp:8007::7 -redir tcp:8007::7 -redir udp:161::161 -redir udp:162::162 -redir tcp:1256::1256

SET Q_PORTS= -serial file:serial0.log

rem SET Q_AHCI=-drive id=disk,file=ahci.img,if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0 

rem SET Q_NET= -net nic,model=ne2k_pci -net nic
SET Q_NET= -net nic,model=ne2k_pci -net user -tftp ./tftp

rem SET Q_NET= -net nic,model=pcnet -net nic,model=rtl8139  -net user -tftp ./tftp
rem SET Q_NET= -net nic,model=rtl8139  -net user,tftp=./tftp -tftp ./tftp
rem SET Q_NET= -net nic,model=pcnet  -net user -tftp ./tftp


SET Q_MACHINE=-m 256
rem SET Q_MACHINE=-m 120

rem SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy-hd0.img -hda fat:fat -hdb phantom.img
rem SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy-hd0.img -fdb openwrt-x86-ext2.image.kernel -hda fat:fat -hdb phantom.img
rem SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy-hd0.img  -hda fat:fat -hdb phantom.img
rem SET Q_DISKS=-boot a -no-fd-bootchk -fda raw:img/grubfloppy-hd0.img   -hdb phantom.img
SET Q_DISKS=-boot a -no-fd-bootchk -drive file=img/grubfloppy-hd0.img,index=0,if=floppy,format=raw -drive file=fat:rw:fat,format=raw -drive if=ide,index=1,file=phantom.img,format=raw

rem -fdb kolibri.iso
rem SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy-hd0.img -fdb e2.img -hda fat:fat -hdb phantom.img 
rem SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy-hd0.img -hda fat:fat -hdb phantom.img 

rem SET Q_KQ=-enable-kqemu
rem SET Q_KQ=-enable-kqemu -kernel-kqemu

rem SET Q_VGA=-vga std
rem SET Q_VGA=-vga cirrus
SET Q_VGA=-vga vmware
rem SET Q_VGA=-device cirrus-vga
rem -virtioconsole 1

del serial0.log.old1
ren serial0.log.old serial0.log.old1
ren serial0.log serial0.log.old
rem start /wait 
%QDIR%\%QCMD% -net dump,file=net.dmp -smp 3 %Q_VGA% -gdb tcp::1234,nowait,nodelay,server,ipv4 %Q_KQ% -L %QDIR%\bios %Q_MACHINE% %Q_PORTS% %Q_DISKS% %Q_NET% %VIO% %USB% %SOUND% %Q_AHCI% %Q_REDIR%


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
