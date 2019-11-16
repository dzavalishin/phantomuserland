#!/bin/sh

. ./target_config.sh 


ssh -l$TARGET_USER $TARGET_HOST sudo grub-reboot phantom-os-default

# If grub sticks to booting to Phantom, exec next cmd on target:
#
# grub-editenv /boot/grub/grubenv unset next_entry
#

