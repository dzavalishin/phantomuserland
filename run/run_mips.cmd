SET QDIR=qemu\0.14.1

rem SET Q_MACHINE=-M mipssim -cpu 34Kf -m 512
SET Q_MACHINE=-M mipssim -m 512
rem SET Q_MACHINE=-M malta -m 256

SET SER=-serial file:serial0.log
rem SET SER=-serial vc 

rem SET Q_NET= -net user -tftp ./tftp -net nic,model=smc91c111
SET STOP=-S

rem -append "-d=20 -- -test all"

rm -f serial0.log 
start /wait %QDIR%\qemu-system-mips %Q_MACHINE%  -gdb tcp::1234,nowait,nodelay,server,ipv4 -L %QDIR%/bios -kernel fat/boot/phantom.mips -sd phantom.img %SER% %STOP% %Q_NET% 
rem -mtdblock vio.img 


rem cat qemu/0.14.1/stderr.txt
cat %QDIR%/stderr.txt
