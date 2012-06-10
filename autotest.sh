#!/bin/sh
cd `dirname $0`
export PHANTOM_HOME=`pwd`
export LANG=C
ME=${0##*/}
QEMU=`which kvm || which qemu 2>/dev/null`

# reasonable fallback
[ "$QEMU" ] || QEMU=/usr/bin/qemu-system-i386

# was oldtree/run_test
TEST_DIR=run/test
TFTP_PATH=../tftp

die ( ) {
	[ -s make.log ] && tail make.log
	[ "$1" ] && echo "$*"
	exit 0
}

COMPILE=1
SNAPTEST=1

while [ $# -gt 0 ]
do
	case "$1" in
	-f)	FORCE=1	;;
	-w)	WARN=1
		MSG="No updates for today"
	;;
	-nc)	unset COMPILE	;;
	-ng)	unset DISPLAY	;;
	-ns)	unset SNAPTEST	;;
	esac
	shift
done

cd $PHANTOM_HOME

preserve_log ( ) {
	[ -d ../public_html/. ] || return

	SAVED_LOG=../public_html/serial0.log

	C=9

	while [ $C -gt 0 ]
	do
		T=`expr $C - 1`

		[ -e $SAVED_LOG.$T ] && mv $SAVED_LOG.$T $SAVED_LOG.$C
		C=$T
	done

	[ -e $SAVED_LOG ] && mv $SAVED_LOG $SAVED_LOG.0

	cp $TEST_DIR/serial0.log $SAVED_LOG

	echo "The serial0.log can be viewed at

http://misc.dz.ru/~`whoami`/serial0.log

Previous copies are kept (serial0.log.0 through .9)"
}

# check if another copy is running

RUNNING=`ps xjf | grep $ME | grep -vw "grep\\|$$"`
DEAD=`ps xjf | grep $QEMU | grep -vw "grep\\|$$"`
[ "$RUNNING" ] && {
	(echo "$RUNNING" | grep -q defunct) || \
	(tail -1 $TEST_DIR/serial0.log | grep ^Press) || {
		echo "Another copy is running: $RUNNING"
		exit 0
	}

	echo "Previous test run stalled. Killing qemu..."
	[ "$DEAD" ] && SIG=-KILL
	pkill $SIG $QEMU

	preserve_log
}
#[ -s $0.lock ] && exit 0
#touch $0.lock
#trap "rm $0.lock" 0

LISTENING=`netstat -pl --inet | grep :1234`
[ "$LISTENING" ] && {
	echo "Somebody took my gdb port! $LISTENING"
	exit 0
}

rm -f make.log
make clean > /dev/null 2>&1

# clean unexpected failures
GRUB_MENU=tftp/tftp/menu.lst
svn diff | grep -q "^--- $TEST_DIR/$GRUB_MENU" && \
	rm $TEST_DIR/$GRUB_MENU
SVN_OUT=`svn update`
[ $? -ne 0 -o `echo "$SVN_OUT" | grep -c '^At revision'` -ne 0 ] && {
	[ "$FORCE" ] || die "$MSG"
}

echo "$SVN_OUT"

[ "$COMPILE" ] && {
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
	-M pc -smp 4 -s -boot a -no-reboot \
	-net nic,model=ne2k_pci -net user
	-parallel file:lpt_01.log \
	-serial file:serial0.log \
	-tftp tftp \
	-no-fd-bootchk \
	-fda img/grubfloppy.img \
	-hda snapcopy.img \
	-hdb phantom.img \
	-drive file=vio.img,if=virtio,format=raw \
	-usb -soundhw sb16"
#	-net nic,model=ne2k_isa -M isapc \

#echo "timeout=1

#title=phantom ALL TESTS
#kernel=(nd)/phantom -d 20 -- -test all
#module=(nd)/classes
#module=(nd)/pmod_test
#boot 
#" > $GRUB_MENU

dd if=/dev/zero of=snapcopy.img bs=4096 skip=1 count=1024 2> /dev/null
dd if=/dev/zero of=vio.img bs=4096 skip=1 count=1024 2> /dev/null


$QEMU $QEMU_OPTS

grep -B 10 'Panic\|[^e]fault\|^EIP\|^- \|Stack:\|^T[0-9 ]' serial0.log && die "Phantom test run failed!"
grep 'SVN' serial0.log || die "Phantom test run crashed!"
grep '[Ff][Aa][Ii][Ll]\|TEST\|SKIP' serial0.log
grep 'FINISHED\|done, reboot' serial0.log || die "Phantom test run error!"

[ "$SNAPTEST" ] || exit 0

cp phantom.img phantom.img.orig
cp $GRUB_MENU $GRUB_MENU.orig

trap "mv $GRUB_MENU.orig GRUB_MENU; mv phantom.img.orig phantom.img" 0

echo "
 ============= Now probing snapshots ========================"
echo "timeout=1

title=phantom
kernel=(nd)/phantom -d=10 -- 
module=(nd)/classes
boot 
" > $GRUB_MENU

# before running again
# TODO call ../zero_ph_img.sh 
cp ../phantom.img .
#rm phantom.img
#touch phantom.img
#echo ": zeroing virtual disk..."
#dd bs=4096 seek=0 count=20480 if=/dev/zero of=phantom.img 2> /dev/null
#echo ": instantating superblock..."
#dd conv=nocreat conv=notrunc bs=4096 count=1 seek=16 if=img/phantom.superblock of=phantom.img 2> /dev/null
dd if=/dev/zero of=snapcopy.img bs=4096 skip=1 count=1024 2> /dev/null
echo ": zeroing vio..."
dd if=/dev/zero of=vio.img bs=4096 skip=1 count=1024 2> /dev/null

for pass in 1 2
do
	echo "
===> pass $pass"
	$QEMU $QEMU_OPTS &
	QEMU_PID=$!

	while [ 1 ]
	do
		sleep 2
		ps -p $QEMU_PID >/dev/null || {
			echo "

FATAL! Phantom crashed"
			break
		}
		[ -s serial0.log ] || {
			sleep 10
			[ -s serial0.log ] || {
				echo "

FATAL! Phantom stalled (serial0.log is empty)"
				kill $QEMU_PID
				break
			}
		}
		grep -iq 'snapshot done' serial0.log && break
		grep -q '^\(\. \)\?Panic' serial0.log && {
			sleep 10
			break
		}
	done

	tail -1 serial0.log | grep -q '^Press any' && {
		echo "

FATAL! Phantom stopped (panic)"
		kill $QEMU_PID
	}

	grep -q '^EIP\|^- \|Stack\|^\(\. \)\?Panic\|^T[0-9 ]' serial0.log && {
		grep 'Phantom\|snapshot\|pagelist\|[^e]fault\|^EIP\|^- \|Stack\|^\(\. \)\?Panic\|^T[0-9 ]' serial0.log
		preserve_log
		break
	}

	grep 'snap:\|Snapshot done' serial0.log || {
		echo "
ERROR! No snapshot activity in log! Aborted"
		tail -10 serial0.log
		preserve_log
		break
	}

	ps -p $QEMU_PID >/dev/null || break

	while (ps -p $QEMU_PID >/dev/null)
	do
		sleep 2
		kill $QEMU_PID
	done
done

#mv phantom.img.orig phantom.img
#mv $GRUB_MENU.orig $GRUB_MENU
#rm phantom.img $GRUB_MENU
