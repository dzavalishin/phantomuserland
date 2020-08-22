#!/bin/sh
#
# This script tries make everything and watches for warnings and errors
#
WARN=1
LOGFILE=make.log

cd $(dirname $0)
export PHANTOM_HOME=$(pwd)
export LANG=C

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
		-w)	WARN=warn		;;
		-s)	unset WARN		;;
		-x)	set -x			;;
		*)
			echo "Usage: $0 [-f] [-u] [-w|-s] [-x]
	-f	- foreground (interactive) run
	-w	- show make warnings
	-s	- silent (ignore make warnings)
	-x	- turn on debug output
"
			exit 0
		;;
		esac
		shift
	done
else
#	CRONMODE=1
#	UNATTENDED=-unattended
	exec 1>$0.log 2>&1
fi

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
	exit 2
fi

