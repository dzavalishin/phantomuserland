SET QDIR=qemu\0.14.1

#SET Q_MACHINE=-M mipssim -cpu 34Kf
SET Q_MACHINE=-M mipssim

SET SER=-serial file:serial0.log
rem SET SER=-serial vc 

#SET Q_NET= -net user -tftp ./tftp -net nic,model=smc91c111


SET STOP=-S

rem -append "-d=20 -- -test all"

rem cp P:/projects/phantomos/trunk/phantom/barebone/mips/barebone.elf fat/boot/phantom.mips

rm -f serial0.log 
%QDIR%\qemu-system-mips %Q_MACHINE% -m 512 -s -L %QDIR%/bios -kernel fat/boot/phantom.mips -sd phantom.img -mtdblock vio.img %SER% %STOP% %Q_NET% 

cat qemu/0.14.1/stderr.txt
