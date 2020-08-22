#!/bin/sh
#
# this script makes pre-requisites for ci-runtest.sh and ci-snaptest.sh
#
#set -x			# debug mode until everything works fine
cd $(dirname $0)
export PHANTOM_HOME=$(pwd)
export LANG=C

PASSES=2
GDB_PORT=1235		# get rid of stalled instance by incrementing port no.
#GDB_PORT_LIMIT=1250	# avoid spawning too many stalled instances
GDB_OPTS="-gdb tcp::$GDB_PORT"
#GDB_OPTS="-s"
TEST_DIR=run/test	# relative to PHANTOM_HOME
TFTP_PATH=../fat/boot	# relative to TEST_DIR
DISK_IMG=phantom.img
#DISK_IMG_ORIG=../../oldtree/run_test/$DISK_IMG
LOGFILE=make.log	# start with build log
GRUB_MENU=tftp/tftp/menu.lst
EXIT_CODE=0
WAIT_LAUNCH=60		# seconds to start Phantom
PANIC_AFTER=200		# seconds to wait for tests to finish or snapshot to be completed

if [ -x /usr/libexec/qemu-kvm ] 	# CentOS check
then
	QEMU=/usr/libexec/qemu-kvm
	QEMU_SHARE=/usr/share/qemu-kvm
else
	QEMU=$(which qemu || which kvm)
	QEMU_SHARE=/usr/share/qemu
fi


die ( ) {
	[ -s $LOGFILE ] && {
		# submit all details in CI, show pre-failure condition interactively
		if [ "$SNAP_CI" ]
		then
			cat $LOGFILE | sed 's/[^m]*m//g;s/
//g'
		else
			tail $LOGFILE
		fi
	}
	[ "$1" ] && echo "$*"
	exit 1
}

[ "$QEMU" ] || {
	# try to find custom qemu
	PKG_MGR=$(which rpm || which dpkg)
	case $PKG_MGR in
	*rpm)	QEMU_PKG=$(rpm -q -l qemu-kvm)  ;;
	*dpkg)	QEMU_PKG=$(dpkg -L qemu-kvm)	;;
	*)	die "Couldn't locate package manager at $(uname -a)"	;;
	esac

	QEMU=$(echo "$QEMU_PKG" | grep 'bin/\(qemu\|kvm\|qemu-kvm\)$')

	[ "$QEMU" ] || die "$QEMU_PKG - cannot find qemu/kvm executable"

	QEMU_SHARE=$(echo "$QEMU" | sed 's#bin/.*#share#')
}


[ "$SNAP_CI" ] && {
	COMPILE=1
	unset DISPLAY
	UNATTENDED=-unattended		# let Phantom output its stack in case of panic
}

while [ $# -gt 0 ]
do
	case "$1" in
	-c)	COMPILE=2		;;	# also make clean
	-f)	FOREGROUND=1		;;
	-u*)	UNATTENDED=-unattended	;;	# force running in unattended mode
	-p)
		shift
		[ "$1" -gt 0 ] && PASSES="$1"
	;;
	-ng)	unset DISPLAY	;;
	-v)	VIRTIO=1	;;
	-vv)	VIRTIO=2	;;		# virtio only
	-wl)
		shift
		[ "$1" -gt 0 ] && WAIT_LAUNCH="$1"
	;;
	-wr)
		shift
		[ "$1" -gt 0 ] && PANIC_AFTER="$1"
	;;
	-x)	set -x		;;	# turn on debug output
	*)
		echo "Usage: $0 [-u|-f] [-c] [-p N] [-wl NN] [-wr MM] [-nc] [-ng] [-v] [-x]
	-f	- run in foreground (no need to specify if other command line args presented)
	-u	- run unattended (don't stop on panic for gdb)
	-c	- run 'make all' first (default in CI mode)
	-p N	- make N passes of snapshot test (default: N=2)
	-wl NN	- wait NN seconds for successful launch (and kill test if the limit reached, default is $WAIT_LAUNCH)
	-wr MM	- wait MM seconds for test results (and kill test if the limit reached, default is $PANIC_AFTER)
	-ng	- do not show qemu/kvm window (default in CI mode)
	-v	- use virtio for snaps
	-vv	- use only virtio (no IDE drives)
	-x	- turn on debug output
"
		exit 0
	;;
	esac
	shift
done

[ "$COMPILE" ] && {
	[ "$COMPILE" = 2 ] && {
		make clean > $LOGFILE 2>&1 || die "cleaning failure"
	}
	make all > $LOGFILE 2>&1 || die "Build failure"
	grep -B1 'error:\|] Error' $LOGFILE && {
		grep '^--- kernel build finished' $LOGFILE || die "Build failure"
	}

	[ "$WARN" ] && {
		echo Warnings:
		grep : $LOGFILE
	}

	# now test building of Phantom library
	if (make -C plib > $LOGFILE 2>&1)
	then
		[ "$WARN" ] && {
			echo Successfully built Phantom library
			grep Warning: $LOGFILE
		}
	else
		echo "Phantom library build failure:"
		tail -20 $LOGFILE
	fi
}

LOGFILE=serial0.log

launch_phantom ( ) {
	rm -f qemu.log gdb.log

	# now prepare control pipes for pseudo-interactive run of qemu and gdb
	QEMUCTL=$(mktemp)
	exec 3>> $QEMUCTL

	GDBCTL=$(mktemp)
	exec 4>> $GDBCTL

	tail -f $QEMUCTL --pid=$$ | $QEMU $QEMU_OPTS > qemu.log &
	QEMU_PID=$!
	echo QEMU control pipe is $QEMUCTL

	# wait for Phantom to start
	ELAPSED=2
	sleep 2
	#echo 'info qtree' >&3	# show setup

	while [ $ELAPSED -lt $WAIT_LAUNCH ]
	do
		[ -s $LOGFILE ] && break

		sleep 2
		kill -0 $QEMU_PID || break
		ELAPSED=$(expr $ELAPSED + 2)
	done

	if [ -s $LOGFILE ]
	then
		tail -f $GDBCTL --pid=$$ | gdb -cd=$PHANTOM_HOME -q > gdb.log &
		echo GDB control pipe is $GDBCTL
	else
		ELAPSED=$PANIC_AFTER
#		LOG_MESSAGE="$LOGFILE is empty"
	fi
}

call_gdb ( ) {
	echo "

FATAL! Phantom stopped (panic)"
	echo 'bt full' >&4

	[ "$SNAP_CI" ] || {
		# restore files
		mv ${GRUB_MENU}.orig $GRUB_MENU

		if [ "$UNATTENDED" ]
		then
			tail -n +5 gdb.log	# show gdb output
		else
			echo "Entering interactive gdb. Type 'q' to quit"
			tail -n +5 -f gdb.log --pid=$$ &	# show gdb output
			while :
			do
				read gdb_cmd

				case "$gdb_cmd" in
				q|quit)	break			;;
				*)	echo "$gdb_cmd" >&4	;;
				esac
			done
		fi
	}

	echo 'q' >&4
}

# setup GDB
echo "
set confirm off
set pagination off
symbol-file oldtree/kernel/phantom/phantom.pe
dir oldtree/kernel/phantom
dir phantom/vm
dir phantom/libc
dir phantom/libc/ia32
dir phantom/dev
dir phantom/libphantom
dir phantom/newos
dir phantom/threads

target remote localhost:$GDB_PORT
b panic
continue
" > .gdbinit

[ "$SNAP_CI" ] && echo "add-auto-load-safe-path $PHANTOM_HOME/.gdbinit" >> $SNAP_CACHE_DIR/.gdbinit

cd $TEST_DIR

# TODO: implement this part in Makefile
for module in phantom classes pmod_test
do
	[ -s tftp/$module ] || {
		cp $TFTP_PATH/$module tftp/
		continue
	}

	[ tftp/$module -ot $TFTP_PATH/$module ] || continue

	echo "$module is renewed"
	cp $TFTP_PATH/$module tftp/
done

rm -f $LOGFILE

[ "$DISPLAY" ] && GRAPH="-vga cirrus -monitor stdio" || GRAPH=-nographic

[ "$VIRTIO" = 2 ] || IDE_DISKS="	-hda snapcopy.img \
	-hdb $DISK_IMG"

QEMU_OPTS="-L $QEMU_SHARE $GRAPH -name Phantom \
	-M pc -smp 4 $GDB_OPTS -boot a -no-reboot \
	-net nic,model=ne2k_pci -net user \
	-parallel file:lpt_01.log \
	-serial file:serial0.log \
	-tftp tftp \
	-no-fd-bootchk \
	-fda img/grubfloppy.img \
	$IDE_DISKS \
	-drive file=vio.img,if=virtio,format=raw \
	-usb -usbdevice mouse \
	-soundhw all"
#	 -usbdevice disk:format=raw:$DISK_IMG \
#	-usbdevice host:0930:1319 \
#	-usbdevice host:08ff:168b \
#	-usbdevice host:1bcf:288e \
#	-usbdevice host:8087:07da \
#	-net dump,file=net.dmp \
#	-net nic,model=ne2k_isa -M isapc \

[ "$SNAP_CI" = true ] || cp $GRUB_MENU ${GRUB_MENU}.orig
