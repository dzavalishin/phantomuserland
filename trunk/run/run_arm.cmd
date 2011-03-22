SET QDIR=qemu\0.13.0

SET SER=-serial file:serial0.log
rem SET SER=-serial vc 

rem -S
%QDIR%\qemu-system-arm -s -L %QDIR%/bios -kernel fat/boot/phantom %SER%
rem qemu-system-arm -kernel zImage.integrator -initrd arm_root.img
