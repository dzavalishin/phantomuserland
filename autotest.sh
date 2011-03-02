#!/bin/sh
export PHANTOM_HOME=`dirname $0`
export LANG=C
ME=${0##*/}

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
	-w)	WARN=1	;;
	-v)	MSG="No updates for today" ;;
	-nc)	unset COMPILE	;;
	-ng)	unset DISPLAY	;;
	-ns)	unset SNAPTEST	;;
	esac
	shift
done

cd $PHANTOM_HOME

# check if another copy is running

RUNNING=`ps xjf | grep $ME | grep -vw "grep\\|$$"`
[ "$RUNNING" ] && {
	echo "Another copy is running: $RUNNING"
	exit 0
}
#[ -s $0.lock ] && exit 0
#touch $0.lock
#trap "rm $0.lock" 0

rm -f make.log
#make clean > /dev/null 2>&1
./build.sh clean > /dev/null 2>&1

# clean unexpected failures
GRUB_MENU=tftp/tftp/menu.lst
svn diff | grep -q "^--- oldtree/run_test/$GRUB_MENU" && \
	rm oldtree/run_test/$GRUB_MENU
SVN_OUT=`svn update`
[ $? -ne 0 -o `echo "$SVN_OUT" | grep -c '^At revision'` -ne 0 ] && {
	[ "$FORCE" ] || die "$MSG"
}

echo "$SVN_OUT"

[ "$COMPILE" ] && {
	./build.sh > make.log 2>&1 || die "Make failure"
	#make -C phantom/vm	>> make.log 2>&1 || die "Make failure in vm"
	#make all > make.log 2>&1 || die "Make failure"
	#make -C phantom/dev	>> make.log 2>&1 || die "Make failure in dev"
	#make -C phantom/newos	>> make.log 2>&1 || die "Make failure in newos"
	#make -C phantom/threads	>> make.log 2>&1 || die "Make failure in threads"
	#make all >> make.log 2>&1 || die "Make failure"
	[ "$WARN" ] && grep : make.log

	tail make.log
}

cd oldtree/run_test
cp ../run/tftp/phantom tftp/

for module in classes pmod_test pmod_tcpdemo
do
	[ -s tftp/$module ] || {
		cp ../run/tftp/$module tftp/
		continue
	}

	[ tftp/$module -ot ../run/tftp/$module ] || continue

	echo "$module is renewed"
	cp ../run/tftp/$module tftp/
done

rm -f serial0.log

[ "$DISPLAY" ] && GRAPH="-vga cirrus" || GRAPH=-nographic
#GRAPH=-nographic

QEMU_OPTS="-L /usr/share/qemu $GRAPH \
	-M pc -smp 4 -s -boot a -no-reboot \
	-net nic,model=pcnet -net user
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

qemu $QEMU_OPTS

grep -iB 10 'Panic\|[^e]fault\|^EIP\|^- \|Stack:\|^T[0-9 ]' serial0.log && die "Phantom test run failed!"
grep 'SVN' serial0.log || die "Phantom test run crashed!"
grep '[Ff][Aa][Ii][Ll]\|TEST' serial0.log
grep 'FINISHED\|done, reboot' serial0.log || die "Phantom test run error!"

[ "$SNAPTEST" ] || exit 0

cp phantom.img phantom.img.orig
cp $GRUB_MENU $GRUB_MENU.orig

trap "mv $GRUB_MENU.orig GRUB_MENU; mv phantom.img.orig phantom.img" 0

echo " ============= Now probing snapshots ========================"
echo "timeout=1

title=phantom
kernel=(nd)/phantom -d=10 -- 
module=(nd)/classes
boot 
" > $GRUB_MENU

# before running again
#touch phantom.img
#dd bs=4096 count=1 if=img/phantom.superblock of=phantom.img
#dd conv=nocreat bs=4096 seek=1 count=20480 if=/dev/zero of=phantom.img
#dd if=/dev/zero of=snapcopy.img bs=4096 skip=1 count=1024
#dd if=/dev/zero of=vio.img bs=4096 skip=1 count=1024
cp ../run/phantom.img .

for pass in 1 2
do
	echo "===> pass $pass"
	qemu $QEMU_OPTS &
	QEMU_PID=$!

	while [ 1 ]
	do
		sleep 2
		ps -p $QEMU_PID >/dev/null || {
			echo "FATAL! Phantom crashed"
			break
		}
		grep -i 'snapshot done' serial0.log && break
	done

	grep 'Phantom\|snapshot\|pagelist\|[^e]fault\|^EIP\|^- \|Stack\|^Panic\|^T[0-9 ]' serial0.log
	ps -p $QEMU_PID || break

	while (ps -p $QEMU_PID >/dev/null)
	do
		sleep 2
		kill $QEMU_PID
	done
done

#mv phantom.img.orig phantom.img
#mv $GRUB_MENU.orig $GRUB_MENU
#rm phantom.img $GRUB_MENU
