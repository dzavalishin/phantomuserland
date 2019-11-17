#!/bin/sh

. ./target_config.sh 


echo "Will FORMAT target system '$TARGET_HOST' disk '$TARGET_PHANTOM_DEV' into the Phantom OS disk format"
echo "Press Enter to continue or ^C to interrupt"

read -r input

ssh -l$TARGET_USER $TARGET_HOST sudo $TARGET_BOOT_DIR/mkfs_phantom.sh $TARGET_PHANTOM_DEV

