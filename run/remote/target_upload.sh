#!/bin/sh

. ./target_config.sh 

# /boot/grub/custom.cfg

scp target/custom.cfg $TARGET_USER@$TARGET_HOST:/boot/grub/custom.cfg
#scp ../img/phantom.superblock $TARGET_USER@$TARGET_HOST:$TARGET_BOOT_DIR
#scp ../fat/boot/phantom.ia32 $TARGET_USER@$TARGET_HOST:$TARGET_BOOT_DIR
#scp ../fat/boot/classes $TARGET_USER@$TARGET_HOST:$TARGET_BOOT_DIR

scp ../img/phantom.superblock ../fat/boot/phantom.ia32 ../fat/boot/classes $TARGET_USER@$TARGET_HOST:$TARGET_BOOT_DIR
