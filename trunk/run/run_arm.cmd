SET QDIR=qemu\0.14.1

SET SER=-serial file:serial0.log
rem SET SER=-serial vc 

SET Q_NET= -net user -tftp ./tftp -net nic,model=smc91c111


#SET STOP=-S

rem -append "-d=20 -- -test all"

rm -f serial0.log 
%QDIR%\qemu-system-arm -s -L %QDIR%/bios -kernel fat/boot/phantom -sd phantom.img -mtdblock vio.img %SER% %STOP% %Q_NET% 
rem -writeconfig arm.cfg

rem qemu-system-arm -kernel zImage.integrator -initrd arm_root.img
