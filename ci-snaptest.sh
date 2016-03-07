#!/bin/sh
#
# this script only runs snapshot test
#
cd `dirname $0`
export PHANTOM_HOME=`pwd`
export LANG=C

PASSES=2
GDB_PORT=1235		# get rid of stalled instance by incrementing port no.
GDB_PORT_LIMIT=1250	# avoid spawning too many stalled instances
GDB_OPTS="-gdb tcp::$GDB_PORT"
#GDB_OPTS="-s"
QEMU=`which qemu || which kvm`
TEST_DIR=run/test	# was oldtree/run_test
TFTP_PATH=../fat/boot
DISK_IMG=phantom.img
LOGFILE=serial0.log
GRUB_MENU=tftp/tftp/menu.lst


die ( ) {
	[ -s $LOGFILE ] && tail $LOGFILE
	[ "$1" ] && echo "$*"
	exit 1
}

if [ $# -gt 0 ]
then
	while [ $# -gt 0 ]
	do
		case "$1" in
		-f)	FOREGROUND=1		;;
		-u)	UNATTENDED=-unattended	;;
		-p)
			shift
			[ "$1" -gt 0 ] && PASSES="$1"
		;;
		-ng)	unset DISPLAY	;;
		-v)	VIRTIO=1	;;
		*)
			echo "Usage: $0 [-u|-f] [-p N] [-ng] [-v]
	-f	- run in foreground (no need to specify if other command line args presented)
	-u	- run unattended (don't stop on panic for gdb)
	-p N	- make N passes of snapshot test (default: N=2)
	-ng	- do not show qemu/kvm window
	-v	- use virtio for snaps
"
			exit 0
		;;
		esac
		shift
	done
else
	CRONMODE=1
	UNATTENDED=-unattended
	exec 1>$0.log 2>&1
fi

call_gdb ( ) {
	[ "$UNATTENDED" ] || {
		echo "GAME OVER. Press Enter to start GDB..."
		read n
	}
	port="${1:-$GDB_PORT}"
	shift
	pid="$1"
	shift

	# restore files
	mv ${DISK_IMG}.orig $DISK_IMG
	mv ${GRUB_MENU}.orig $GRUB_MENU

	cd $PHANTOM_HOME
	echo "

FATAL! Phantom stopped (panic)"
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

target remote localhost:$port

bt full
quit
" > .gdbinit
	gdb

	[ "$1" ] && echo "$*"
	[ "$pid" ] && kill -9 $pid

	exit 1
}

cd $TEST_DIR

for module in phantom classes
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

[ "$DISPLAY" ] && GRAPH="-vga cirrus" || GRAPH=-nographic
#GRAPH=-nographic

QEMU_OPTS="-L /usr/share/qemu $GRAPH \
	-M pc -smp 4 $GDB_OPTS -boot a -no-reboot \
	-net nic,model=ne2k_pci -net user \
	-parallel file:lpt_01.log \
	-serial file:serial0.log \
	-tftp tftp \
	-no-fd-bootchk \
	-fda img/grubfloppy.img \
	-hda snapcopy.img \
	-hdb $DISK_IMG \
	-drive file=vio.img,if=virtio,format=raw \
	-usb -soundhw sb16"
#	-net dump,file=net.dmp \
#	-net nic,model=ne2k_isa -M isapc \

cp $DISK_IMG ${DISK_IMG}.orig
cp $GRUB_MENU ${GRUB_MENU}.orig

echo "color yellow/blue yellow/magenta

timeout=3

title=phantom
kernel=(nd)/phantom -d=10 $UNATTENDED -- 
module=(nd)/classes
boot 
" > $GRUB_MENU

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
	cp ../$DISK_IMG vio.img
	echo ": zeroing phantom drive ..."
	dd if=/dev/zero of=phantom.img bs=4096 skip=1 count=1024 2> /dev/null
else
	cp ../$DISK_IMG .
	echo ": zeroing vio..."
	dd if=/dev/zero of=vio.img bs=4096 skip=1 count=1024 2> /dev/null
fi

for pass in `seq 1 $PASSES`
do
	echo "
===> pass $pass"
	$QEMU $QEMU_OPTS &
	QEMU_PID=$!

	while [ 1 ]
	do
		sleep 2
		kill -0 $QEMU_PID || {
			echo "

FATAL! Phantom snapshot test crashed"
			break
		}
		[ -s $LOGFILE ] || {
			sleep 30
			[ -s $LOGFILE ] || {
				echo "

FATAL! Phantom snapshot test stalled ($LOGFILE is empty)"
				kill $QEMU_PID
				break
			}
		}
		grep -iq 'snapshot done' $LOGFILE && break
		grep -q '^\(\. \)\?Panic' $LOGFILE && {
			sleep 15
			break
		}
	done

	tail -1 $LOGFILE | grep -q '^Press any' && \
		call_gdb $GDB_PORT $QEMU_PID "Pass $pass panic" 

	grep -q '^EIP\|^- \|Stack\|^\(\. \)\?Panic\|^T[0-9 ]' $LOGFILE && {
		# show extract from the log here
		grep 'Phantom\|snapshot\|pagelist\|[^e]fault\|^EIP\|^- \|Stack\|^\(\. \)\?Panic\|^T[0-9 ]' $LOGFILE
		#preserve_log serial0.log
		break
	}

	grep 'snap:\|Snapshot done' $LOGFILE || {
		echo "
ERROR! No snapshot activity in log! Phantom snapshot test failed"
		tail -10 $LOGFILE
		#preserve_log $LOGFILE
		break
	}

	kill -0 $QEMU_PID >/dev/null || break

	while (ps -p $QEMU_PID >/dev/null)
	do
		sleep 2
		kill $QEMU_PID
	done
done

mv ${DISK_IMG}.orig $DISK_IMG
mv ${GRUB_MENU}.orig $GRUB_MENU
##rm $DISK_IMG $GRUB_MENU
