#!/bin/bash

#phantom_boot.img
IMAGE=bootfat.img



echo "Making phantom_boot_fat.img image, using content of fat folder"
cp -r ./grub/* ./fat/
touch ./$IMAGE
dd if=/dev/zero of=./$IMAGE bs=4096 skip=1 count=20240
makebootfat -o ./$IMAGE -b img/ldlinux.bss -m grub/grub4dos/grldr.mbr -c grub/grub4dos/grldr  fat


#sudo chown $USER $IMAGE
