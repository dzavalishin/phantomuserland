#!/bin/sh

. ./target_config.sh 

#grub-reboot phantom-os-default

# grub-editenv /boot/grub/grubenv unset next_entry

ssh -l$TARGET_USER $TARGET_HOST sudo reboot
