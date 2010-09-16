rem SOUND=-soundhw sb16

rem SET USB=-usb -usbdevice mouse
rem SET USB=-usb 
SET VIO=-drive file=vio.img,if=virtio,format=raw

rem SET SCSI=-drive file=scsi.img,if=scsi,unit=0

rem SET Q_PORTS= -parallel file:lpt_01.log  -serial file:serial0.log
SET Q_PORTS= -serial file:serial0.log

rem SET Q_NET=-net nic,model=ne2k_isa -net nic,model=ne2k_pci
rem    -net nic,model=ne2k_isa -net nic,model=rtl8139 -net nic,model=i82559er -net nic,model=pcnet -net nic,model=ne2k_isa
SET Q_NET= -net nic,model=pcnet  -net user -tftp ../run/tftp
rem SET Q_NET= -net nic,model=virtio -net nic,model=pcnet -net nic,model=ne2k_pci  -net user -tftp tftp

rem SET Q_MACHINE=-M isapc
rem SET Q_MACHINE=-m 85

SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy.img -hda snapcopy.img -hdb phantom.img 
rem -hdc cd.iso

rem SET Q_KQ=-enable-kqemu
rem SET Q_KQ=-enable-kqemu -kernel-kqemu
rem SET DEBUG=

rem SET Q_VGA=-vga std
SET Q_VGA=-vga cirrus
rem SET Q_VGA=-vga vmware
rem -virtioconsole 4

rem rm serial0.log.old
rem mv serial0.log serial0.log.old
del serial0.log.old1
ren serial0.log.old serial0.log.old1
ren serial0.log serial0.log.old
bin\qemu -smp 3 %Q_VGA% -s %Q_KQ% -L lib %Q_MACHINE% %Q_PORTS%  %Q_DISKS%  %Q_NET% %VIO% %USB% %SOUND% -writeconfig qemu.cfg

exit
