#!/bin/sh
#
# This script tries make everything and watches for warnings and errors
#
WARN=1
LOGFILE=make.log

cd `dirname $0`
export LANG=C

die ( ) {
	[ -s $LOGFILE ] && tail $LOGFILE
	[ "$1" ] && echo "$*"
	exit 1
}

[ "$1" = "-s" ] && unset WARN	# ignore warnings in silent mode

make clean > /dev/null 2>&1
rm -f $LOGFILE			# sanitize against wrong links etc

#./build.sh > make.log 2>&1 || die "Make failure"
#make -C phantom/vm	>> make.log 2>&1 || die "Make failure in vm"
#make all > make.log 2>&1 || die "Make failure"
#make -C phantom/dev	>> make.log 2>&1 || die "Make failure in dev"
#make -C phantom/newos	>> make.log 2>&1 || die "Make failure in newos"
#make -C phantom/threads	>> make.log 2>&1 || die "Make failure in threads"
make all > $LOGFILE 2>&1 || die "Make failure"
grep -B1 'error:\|] Error' $LOGFILE && {
	grep -q '^--- kernel build finished' $LOGFILE || die "Make failure"
}

[ "$WARN" ] && {
	echo Warnings:
	grep : $LOGFILE
}

tail $LOGFILE
