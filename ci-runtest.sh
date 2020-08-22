#!/bin/sh
#
# This script only executes test mode
#
. ./ci-common.sh	# ci-runtest and ci-snaptest have much in common

#PANIC_AFTER=200		# kill test after 200 seconds of inactivity

echo "color yellow/blue yellow/magenta
timeout=3

title=phantom ALL TESTS
kernel=(nd)/phantom -d=20 $UNATTENDED -- -test all
module=(nd)/classes
module=(nd)/pmod_test
boot 
" > $GRUB_MENU

dd if=/dev/zero of=snapcopy.img bs=4096 skip=1 count=1024 2> /dev/null
dd if=/dev/zero of=vio.img bs=4096 skip=1 count=1024 2> /dev/null

# take working copy of the Phantom disk
cp ../../run/test/img/$DISK_IMG .

launch_phantom
#echo 'info pci' >&3

while [ $ELAPSED -lt $PANIC_AFTER ]
do

	sleep 2
	kill -0 $QEMU_PID || break
	ELAPSED=`expr $ELAPSED + 2`

	(tail -5 gdb.log | grep '^Breakpoint 1[, ]') && {
		call_gdb
		EXIT_CODE=2
		break
	}
done

[ "$EXIT_CODE" = 2 ] || echo 'quit' >&4		# terminate gdb
rm $QEMUCTL $GDBCTL

# check if finished in time
[ $ELAPSED -lt $PANIC_AFTER ] || {
	echo "

FATAL! Phantom test stalled: ${LOG_MESSAGE:-no activity after $PANIC_AFTER seconds}"
	(ps -p $QEMU_PID >/dev/null) && echo 'quit' >&3	# stop emulation
	EXIT_CODE=3
}

[ "$SNAP_CI" ] || {
	grep -q 'TEST FAILED' $LOGFILE && {
		cp $LOGFILE test.log
		#preserve_log test.log
	}
	mv ${GRUB_MENU}.orig $GRUB_MENU
}

# perform final checks
grep -B 10 'Panic\|[^e]fault\|^EIP\|^- \|Stack:\|^T[0-9 ]' $LOGFILE && die "Phantom test run failed!"
grep 'SVN' $LOGFILE || die "Phantom test run crashed!"
# show test summary in output
grep '[Ff][Aa][Ii][Ll]\|TEST\|SKIP' $LOGFILE
grep 'FINISHED\|done, reboot' $LOGFILE || die "Phantom test run error!"

# submit all details into the CI log, cutting off ESC-codes
[ "$SNAP_CI" ] && cat qemu.log $LOGFILE | sed 's/[^m]*m//g'
exit $EXIT_CODE
