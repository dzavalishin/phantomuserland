#!/bin/sh

# Machine to upload to
TARGET_HOST=ubuntu.

# User on target machine
TARGET_USER=phantom

# Directory on target machine which is accessible 
# as `/boot/phantom/` to grub at boot time
#
# Note that `set root=(hd1,msdos7)` in `target/custom.cfg` must
# point to disk which is mounted at `/mnt/boot`
#
TARGET_BOOT_DIR=/mnt/boot/phantom/

TARGET_CLASS_DIR=/mnt/files/class/

# Device on target computer that contains Phantom OS
# disk image.
#

TARGET_PHANTOM_DEV=/dev/sbd2
