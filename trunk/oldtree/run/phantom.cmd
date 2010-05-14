SET VIO=-drive file=vio.img,if=virtio,format=raw

SET Q_PORTS= -parallel file:lpt_01.log  -serial file:serial0.log

rem SET Q_NET=-net nic,model=ne2k_isa -net user -tftp /tftp
rem -net nic,model=rtl8139
SET Q_NET= -net nic,model=virtio -net nic,model=pcnet -net nic,model=ne2k_isa  -net user -tftp tftp

rem SET Q_MACHINE=-M isapc
SET Q_MACHINE=

SET Q_DISKS=-boot a -no-fd-bootchk -fda img/grubfloppy.img -hda snapcopy.img -hdb phantom.img 
rem -hdc cd.iso

SET Q_KQ= -no-kqemu
rem SET Q_KQ=-kernel-kqemu
rem SET DEBUG=

SET Q_VGA=-vga std
rem SET Q_VGA=-vga vmware
SET Q_VGA=-vga cirrus
rem SET Q_VGA=-vga bochs
rem -virtioconsole 4

rm serial0.log.old
mv serial0.log serial0.log.old
bin\qemu %Q_VGA% -s %Q_KQ% -L lib %Q_MACHINE% %Q_PORTS%  %Q_DISKS%  %Q_NET% %VIO%
