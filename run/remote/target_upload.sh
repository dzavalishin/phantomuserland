#!/bin/sh

. ./target_config.sh 

scp target/custom.cfg $TARGET_USER@$TARGET_HOST:/boot/grub/custom.cfg
scp ../img/phantom.superblock ../fat/boot/phantom.ia32 ../fat/boot/classes ../fat/boot/pmod_* target/mkfs_phantom.sh $TARGET_USER@$TARGET_HOST:$TARGET_BOOT_DIR
scp ../fat/class/* $TARGET_USER@$TARGET_HOST:$TARGET_CLASS_DIR
