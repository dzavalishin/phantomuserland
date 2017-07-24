#!/bin/bash

echo "Making phantom_boot.img image, using content of fat folder"
cp -r ./grub/* ./fat/
sudo grub-mkrescue -o ./phantom_boot.img --modules="font gfxterm echo reboot usb_keyboard multiboot fat ls cat ext2 iso9660 reiserfs xfs part_sun part_gpt part_msdos" ./fat/

sudo chown $USER phantom_boot.img
