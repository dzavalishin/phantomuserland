#!/bin/sh

. ./target_config.sh 

# /boot/grub/custom.cfg

scp ../img/phantom.superblock $TARGET_USER@$TARGET_HOST:/mnt/boot/phantom
scp target/custom.cfg $TARGET_USER@$TARGET_HOST:/boot/grub/custom.cfg
scp ../fat/boot/phantom.ia32 $TARGET_USER@$TARGET_HOST:/mnt/boot/phantom/
scp ../fat/boot/classes $TARGET_USER@$TARGET_HOST:/mnt/boot/phantom/
