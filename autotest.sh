#!/bin/sh
cd `dirname $0`
export PHANTOM_HOME=`pwd`
export LANG=C
ME=${0##*/}
PASSES=2
GDB_PORT=1235		# get rid of stalled instance by incrementing port no.
GDB_PORT_LIMIT=1250	# avoid spawning too many stalled instances
GDB_OPTS="-gdb tcp::$GDB_PORT"
#GDB_OPTS="-s"
QEMU=`which qemu`		# qemu 0.x
[ "$QEMU" ] || QEMU=`which kvm`	# qemu 1.x and later

TEST_DIR=run/test	# was oldtree/run_test
TFTP_PATH=../fat/boot
DISK_IMG=phantom.img
PHANTOM_LOG=$PHANTOM_HOME/$TEST_DIR/serial0.log

die ( ) {
	[ -s make.log ] && tail make.log
	[ "$1" ] && echo "$*"
	exit 0
}

COMPILE=1
SNAPTEST=1
TESTRUN=1

at_exit ( ) {
	[ "$RESTORE_IMG" ] && mv $DISK_IMG.orig $DISK_IMG
	[ "$CRONMODE" ] && grep -qv svn $0.log >/dev/null && {
		if [ make.log -nt $PHANTOM_LOG ]
		then
			VERSION=`grep revision $0.log`
			RESULT=`grep Error make.log | head -1`
			SEND_LOG=make.log
		else
			VERSION=`grep SVN $PHANTOM_LOG | sed 's/[^m]*m//g;s/starting//'`
			RESULT=`grep 'FAIL\|Panic\|snapshot test' $PHANTOM_LOG | tr '\n' ';'`
			SEND_LOG="$0.log $PHANTOM_LOG"
		fi
		sed 's/[^m]*m//g;s///g' $SEND_LOG | mail -s "$VERSION: ${RESULT:-test ok}" ${MAILTO:-`whoami`}
	}
}

[ $# -gt 0 ] || {
	CRONMODE=1
	UNATTENDED=-unattended
	exec 1>$0.log 2>&1
}

while [ $# -gt 0 ]
do
	case "$1" in
	-f)	FORCE=1			;;
	-u)	UNATTENDED=-unattended	;;
	-p)
		shift
		[ "$1" -gt 0 ] && PASSES="$1"
	;;
	-w)	WARN=1
		MSG="No updates for today"
	;;
	-nc)	unset COMPILE	;;
	-ng)	unset DISPLAY	;;
	-ns)	unset SNAPTEST	;;
	-nt)	unset TESTRUN	;;
	-v)	VIRTIO=1	;;
	*)
		echo "Usage: $0 [-f] [-u] [-p N] [-w] [-nc] [-ng] [-ns]
	-f	- force test (ignore no updates from repository)
	-u	- run unattended (don't stop on panic for gdb)
	-p N	- make N passes of snapshot test (default: N=2)
	-w	- show make warnings
	-nc	- do not compile (run previously compiled version)
	-nt	- do not run test suite
	-ns	- do not run snapshot test (shortcut for -p 0)
	-ng	- do not show qemu/kvm window
	-v	- use virtio for snaps
"
		exit 0
	;;
	esac
	shift
done

cd $PHANTOM_HOME

preserve_log ( ) {
	[ -d ../public_html/. ] || return

	SAVED_LOG=../public_html/$1

	C=9

	while [ $C -gt 0 ]
	do
		T=`expr $C - 1`

		[ -e $SAVED_LOG.$T ] && mv $SAVED_LOG.$T $SAVED_LOG.$C
		C=$T
	done

	[ -e $SAVED_LOG ] && mv $SAVED_LOG $SAVED_LOG.0

	cp $TEST_DIR/$1 $SAVED_LOG

	echo "The $1 can be viewed at

http://misc.dz.ru/~`whoami`/$1

Previous copies are kept ($1.0 through .9)"
}

call_gdb ( ) {
	[ "$UNATTENDED" ] || {
		echo "GAME OVER. Press Enter to start GDB..."
		read n
	}
	port="${1:-$GDB_PORT}"
	shift
	pid="$1"
	shift
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
	exit 0
}

# clean unexpected failures
[ "$COMPILE" ] && rm -f make.log

# update data BEFORE checking for stalled copies
GRUB_MENU=tftp/tftp/menu.lst
git diff | grep -q "^--- $TEST_DIR/$GRUB_MENU" && \
	rm $TEST_DIR/$GRUB_MENU
GIT_OUT=`git pull`
[ $? -ne 0 -o `echo "$GIT_OUT" | grep -c '^Already up-to-date'` -ne 0 ] && {
	[ "$FORCE" ] || die "$MSG"
}

echo "$GIT_OUT"

# check if another copy is running
[ "$FORCE" ] || {
	RUNNING=`ps xjf | grep $ME | grep -vw "grep\\|$$"`
	DEAD=`ps xjf | grep $QEMU | grep -vw "grep"`
	[ "$RUNNING" ] && {
		(echo "$RUNNING" | grep -q defunct) || \
		(tail -1 $PHANTOM_LOG | grep ^Press) || {
			echo "Another copy is running: $RUNNING"
			tail -10 $PHANTOM_LOG
			exit 0
		}

		echo "$RUNNING
$DEAD
Previous test run stalled. Trying gdb..."
		call_gdb $GDB_PORT `echo "$DEAD" | awk '{ print $2 }'`

		preserve_log serial0.log
	}
#[ -s $0.lock ] && exit 0
#touch $0.lock

	while [ "`netstat -pl --inet 2>/dev/null | grep :$GDB_PORT`" ]
	do
		echo "Somebody took my gdb port! Taking next one..."
		GDB_PORT=`expr $GDB_PORT + 1`
		[ $GDB_PORT -gt $GDB_PORT_LIMIT ] && {
			echo "Too many attempts. Aborted"
			exit 0
		}
	done
}

# now it is safe to alter behaviour on exit
trap at_exit 0 2

[ "$COMPILE" ] && {
	make clean > /dev/null 2>&1

	#./build.sh > make.log 2>&1 || die "Make failure"
	#make -C phantom/vm	>> make.log 2>&1 || die "Make failure in vm"
	#make all > make.log 2>&1 || die "Make failure"
	#make -C phantom/dev	>> make.log 2>&1 || die "Make failure in dev"
	#make -C phantom/newos	>> make.log 2>&1 || die "Make failure in newos"
	#make -C phantom/threads	>> make.log 2>&1 || die "Make failure in threads"
	make all >> make.log 2>&1 || die "Make failure"
	grep -B1 'error:\|] Error' make.log && {
		grep -q '^--- kernel build finished' make.log || die "Make failure"
	}
	[ "$WARN" ] && grep : make.log

	tail make.log
}


cd $TEST_DIR
cp $TFTP_PATH/phantom tftp/

for module in classes pmod_test pmod_tcpdemo
do
	[ -s tftp/$module ] || {
		cp $TFTP_PATH/$module tftp/
		continue
	}

	[ tftp/$module -ot $TFTP_PATH/$module ] || continue

	echo "$module is renewed"
	cp $TFTP_PATH/$module tftp/
done

rm -f serial0.log

[ "$DISPLAY" ] && GRAPH="-vga cirrus" || GRAPH=-nographic
#GRAPH=-nographic

QEMU_OPTS="-L /usr/share/qemu $GRAPH \
	-M pc -smp 4 $GDB_OPTS -boot a -no-reboot \
	-net nic,model=ne2k_pci -net user
	-parallel file:lpt_01.log \
	-serial file:serial0.log \
	-tftp tftp \
	-no-fd-bootchk \
	-fda img/grubfloppy.img \
	-hda snapcopy.img \
	-hdb $DISK_IMG \
	-drive file=vio.img,if=virtio,format=raw \
	-usb -soundhw sb16"
#	-net nic,model=ne2k_isa -M isapc \

# isolate test suite to a separate function
test_run ( ) {
	echo "color yellow/blue yellow/magenta
timeout=3

title=phantom ALL TESTS
kernel=(nd)/phantom -d 20 $UNATTENDED -- -test all
module=(nd)/classes
module=(nd)/pmod_test
boot 
" > $GRUB_MENU

	dd if=/dev/zero of=snapcopy.img bs=4096 skip=1 count=1024 2> /dev/null
	dd if=/dev/zero of=vio.img bs=4096 skip=1 count=1024 2> /dev/null

	$QEMU $QEMU_OPTS &
	QEMU_PID=$!

	while [ 1 ]
	do
		sleep 2
		kill -0 $QEMU_PID || break

		[ -s serial0.log ] || {
			sleep 30
			[ -s serial0.log ] || {
				echo "

FATAL! Phantom stalled (serial0.log is empty)"
				kill $QEMU_PID
				break
			}
		}

		tail -1 serial0.log | grep -q '^Press any' && \
			call_gdb $GDB_PORT $QEMU_PID "Test run failed"

		grep -q '^\(\. \)\?Panic' serial0.log && {
			sleep 15
			break
		}
	done

	grep -B 10 'Panic\|[^e]fault\|^EIP\|^- \|Stack:\|^T[0-9 ]' serial0.log && die "Phantom test run failed!"
	grep 'SVN' serial0.log || die "Phantom test run crashed!"
	grep '[Ff][Aa][Ii][Ll]\|TEST\|SKIP' serial0.log
	grep 'FINISHED\|done, reboot' serial0.log || die "Phantom test run error!"
	grep -q 'TEST FAILED' serial0.log && {
		cp serial0.log test.log
		preserve_log test.log
	}
}
# test suite done
[ "$TESTRUN" ] && test_run

[ "$SNAPTEST" ] || exit 0

cp $DISK_IMG $DISK_IMG.orig

RESTORE_IMG=yes

echo "
 ============= Now probing snapshots ========================"
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
		[ -s serial0.log ] || {
			sleep 15
			[ -s serial0.log ] || {
				echo "

FATAL! Phantom snapshot test stalled (serial0.log is empty)"
				kill $QEMU_PID
				break
			}
		}
		grep -iq 'snapshot done' serial0.log && break
		grep -q '^\(\. \)\?Panic' serial0.log && {
			sleep 15
			break
		}
	done

	tail -1 serial0.log | grep -q '^Press any' && \
		call_gdb $GDB_PORT $QEMU_PID "Pass $pass panic" 

	grep -q '^EIP\|^- \|Stack\|^\(\. \)\?Panic\|^T[0-9 ]' serial0.log && {
		grep 'Phantom\|snapshot\|pagelist\|[^e]fault\|^EIP\|^- \|Stack\|^\(\. \)\?Panic\|^T[0-9 ]' serial0.log
		preserve_log serial0.log
		break
	}

	grep 'snap:\|Snapshot done' serial0.log || {
		echo "
ERROR! No snapshot activity in log! Phantom snapshot test failed"
		tail -10 serial0.log
		preserve_log serial0.log
		break
	}

	kill -0 $QEMU_PID >/dev/null || break

	while (ps -p $QEMU_PID >/dev/null)
	do
		sleep 2
		kill $QEMU_PID
	done
done

#mv $DISK_IMG.orig $DISK_IMG
#mv $GRUB_MENU.orig $GRUB_MENU
##rm $DISK_IMG $GRUB_MENU
