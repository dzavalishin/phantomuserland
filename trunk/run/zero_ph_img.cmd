sh zero_ph_img.sh

rem del phantom.img
rem dd if=/dev/zero of=vio.img bs=4096 skip=1 count=1024
rem ..\oldtree\run12\bin\pfsformat phantom.img