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

PANIC_AFTER=1800	# wait 30 minutes for a snapshot

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

set -x	# debug

for pass in `seq 1 $PASSES`
do
	echo "
===> pass $pass"
	tail -f $CTLPIPE --pid=$$ | $QEMU $QEMU_OPTS > qemu.log &
	QEMU_PID=$!
	ELAPSED=0

	while [ $ELAPSED -lt $PANIC_AFTER ]
	do
		sleep 2
		ELAPSED=`expr $ELAPSED + 2`
		# tick every minute to keep CI going
		[ "$SNAP_CI" ] && {
			[ `expr $ELAPSED % 60` -eq 0 ] && {
				M=`expr $ELAPSED / 60`
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
		[ -s $LOGFILE ] || {
			sleep 50
			ELAPSED=`expr $ELAPSED + 50`
			[ -s $LOGFILE ] || {
				echo "

FATAL! Phantom snapshot test stalled ($LOGFILE is empty)"
				kill $QEMU_PID
				EXIT_CODE=2
				break
			}
			echo 'info pci' >&3
		}
		grep -iq 'snapshot done' $LOGFILE && break

		tail -1 $LOGFILE | grep -q '^Press any' && \
			call_gdb $QEMU_PID $GDB_PORT "Pass $pass panic" 

		grep -q '^\(\. \)\?Panic' $LOGFILE && {
			[ "$UNATTENDED" ] && sleep 15	# wait for stack dump
			break
		}
	done

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

	grep 'snap:\|Snapshot done' $LOGFILE || {
		echo "
ERROR! No snapshot activity in log! Phantom snapshot test failed"
		tail -10 $LOGFILE
		EXIT_CODE=4
		#preserve_log $LOGFILE
		break
	}

	(ps -p $QEMU_PID >/dev/null) && echo 'quit' >&3	# stop emulation
	cat qemu.log
done

rm $CTLPIPE

if [ "$SNAP_CI" = true ]
then
	[ $EXIT_CODE -gt 0 ] && exit $EXIT_CODE

	[ "$VIRTIO" ] || {
		echo Now re-run with virtio ...
		cd $PHANTOM_HOME
		case "$0" in		# re-instantiate with VIRTIO
		/*)	. $0 -v		;;
		*)	. ./$0 -v	;;
		esac
	}
else
	mv ${GRUB_MENU}.orig $GRUB_MENU
fi
