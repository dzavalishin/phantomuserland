#!/bin/sh

. ./target_config.sh 

echo "Will get image of Phantom disk from target computer"

ssh -l$TARGET_USER $TARGET_HOST sudo cp $TARGET_PHANTOM_DEV /tmp/phantom.img

scp $TARGET_USER@$TARGET_HOST:/tmp/phantom.img target_phantom.img

ssh -l$TARGET_USER $TARGET_HOST sudo rm /tmp/phantom.img
