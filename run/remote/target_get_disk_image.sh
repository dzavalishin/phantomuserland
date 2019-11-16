#!/bin/sh

. ./target_config.sh 

echo "Will get image of Phantom disk from target computer"

#ssh -lphantom ubuntu. sudo reboot

scp $TARGET_USER@$TARGET_HOST:$TARGET_PHANTOM_DEV target_phantom.img
