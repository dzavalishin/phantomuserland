#!/bin/sh
#
# this script only runs snapshot test
#
. ./ci-common.sh	# ci-runtest and ci-snaptest have much in common

echo "color white/green yellow/magenta

timeout=3

title=phantom
kernel=(nd)/phantom -d=10 $UNATTENDED -- 
module=(nd)/classes
boot 
" > $GRUB_MENU

PANIC_AFTER=500		# wait 5 minutes for a snapshot to be completed

# before running again
# TODO call ../zero_ph_img.sh 
#rm $DISK_IMG
#touch $DISK_IMG
#echo ": zeroing virtual disk..."
#dd bs=4096 seek=0 count=20480 if=/dev/zero of=$DISK_IMG 2> /dev/null
#echo ": instantating superblock..."
#dd conv=nocreat conv=notrunc bs=4096 count=1 seek=16 if=img/phantom.superblock of=$DISK_IMG 2> /dev/null
dd if=/dev/zero of=snapcopy.img bs=4096 skip=1 count=1024 2> /dev/null

if [ "$VIRTIO" ]
then
	cp $DISK_IMG_ORIG vio.img
	[ "$VIRTIO" = 2 ] || {
		echo ": zeroing phantom drive ..."
		dd if=/dev/zero of=phantom.img bs=4096 skip=1 count=1024 2> /dev/null
	}
else
	cp $DISK_IMG_ORIG .
	echo ": zeroing vio..."
	dd if=/dev/zero of=vio.img bs=4096 skip=1 count=1024 2> /dev/null
fi

#set -x	# debug

for pass in $(seq 1 $PASSES)
do
	echo "
===> pass $pass"
	launch_phantom

	while [ $ELAPSED -lt $PANIC_AFTER ]
	do
		sleep 2
		ELAPSED=$(expr $ELAPSED + 2)
		# tick every minute to keep CI going
		[ "$SNAP_CI" ] && {
			[ `expr $ELAPSED % 60` -eq 0 ] && {
				M=$(expr $ELAPSED / 60)
				case $M in
				1) echo "Running $M minute..."	;;
				*) echo "Running $M minutes..."	;;
				esac
			}
		}
		kill -0 $QEMU_PID || {
			echo "

FATAL! Phantom snapshot test crashed"
			EXIT_CODE=3
			break
		}

		(tail -3 gdb.log | grep ^Breakpoint\ 1,) && {
			call_gdb
			EXIT_CODE=2
			break
		}

		grep -iq 'snapshot done' $LOGFILE && break
	done

	rm $QEMUCTL $GDBCTL

	(ps -p $QEMU_PID >/dev/null) && echo 'quit' >&3	# stop emulation
	[ "$SNAP_CI" ] && tail -n +3 qemu.log		# show monitor logs if any

	if [ "$EXIT_CODE" = 2 ]
	then
		break		# get outta here, gdb is dead already
	else
		echo 'quit' >&4	# terminate gdb and proceed further
	fi

	grep -q '^EIP\|^- \|Stack\|^\(\. \)\?Panic\|^T[0-9 ]' $LOGFILE && {
		if [ "$SNAP_CI" = true ]
		then
			# show complete log in CI
			cat $LOGFILE | sed 's/[^m]*m//g;s///g'
		else
			# show extract from the log otherwise
			grep 'Phantom\|snapshot\|pagelist\|[^e]fault\|^EIP\|^- \|Stack\|^\(\. \)\?Panic\|^T[0-9 ]' $LOGFILE
		fi
		EXIT_CODE=3
		#preserve_log serial0.log
		break
	}

	grep 'Snapshot done' $LOGFILE || {
		echo "
ERROR! No snapshot activity in log! Phantom snapshot test failed"
		call_gdb			# just in case there's something interesting there
		if [ "$SNAP_CI" = true ]
		then
			# show complete log in CI
			cat $LOGFILE | sed 's/[^m]*m//g;s///g'
		else
			# show extract from the log otherwise
			tail -10 $LOGFILE
		fi
		EXIT_CODE=4
		#preserve_log $LOGFILE
		break
	}
	grep -i snap $LOGFILE	# show snapshot activity summary
done

if [ "$SNAP_CI" = true ]
then
	[ $EXIT_CODE -gt 0 ] && exit $EXIT_CODE

	[ "$VIRTIO" ] || {
		echo "

Now re-run with virtio ..."
		cd $PHANTOM_HOME
		case "$0" in		# re-instantiate with VIRTIO
		/*)	. $0 -v		;;
		*)	. ./$0 -v	;;
		esac
	}
else
	mv ${GRUB_MENU}.orig $GRUB_MENU
fi
