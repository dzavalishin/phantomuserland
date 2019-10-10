del phantom.img
rem touch phantom.img
rem dd bs=4096 count=1 if=img/phantom.superblock of=phantom.img
rem dd conv=nocreat bs=4096 seek=1 count=20480 if=/dev/zero of=phantom.img 
rem dd if=/dev/zero of=snapcopy.img bs=4096 skip=1 count=1024
rem dd if=/dev/zero of=vio.img bs=4096 skip=1 count=1024

bin\pfsformat phantom.img