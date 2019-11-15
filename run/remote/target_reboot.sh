#!/bin/sh

. ./target_config.sh 

# /boot/grub/custom.cfg

#scp target/custom.cfg $TARGET_USER@$TARGET_HOST:/boot/grub/custom.cfg


#grub-reboot phantom-os-default

# grub-editenv /boot/grub/grubenv unset next_entry

ssh -lphantom ubuntu. sudo reboot

#ssh -l$TARGET_USER@ TARGET_HOST reboot
