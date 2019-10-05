#!/bin/sh
cd $(dirname $0)
export PHANTOM_HOME=$(pwd)
export LANG=C
ME=${0##*/}
PASSES=2
GDB_PORT=1235		# get rid of stalled instance by incrementing port no.
GDB_PORT_LIMIT=1250	# avoid spawning too many stalled instances
#GDB_OPTS="-gdb tcp::$GDB_PORT"
#GDB_OPTS="-s"
QEMU=$(which qemu)			# qemu 0.x
[ "$QEMU" ] || QEMU=$(which kvm)	# qemu 1.x and later

TEST_DIR=run/test	# was oldtree/run_test
#TFTP_PATH=../fat/boot
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
#PANIC_AFTER=180		# abort test after 3 minutes (consider stalled)

at_exit ( ) {
	[ "$RESTORE_IMG" ] && mv $DISK_IMG.orig $DISK_IMG
	[ "$CRONMODE" ] && grep -qv svn $0.log >/dev/null && {
		if [ make.log -nt $PHANTOM_LOG ]
		then
			VERSION=$(grep revision $0.log)
			RESULT=$(grep Error make.log | head -1)
			SEND_LOG=make.log
		else
			VERSION=$(grep SVN $PHANTOM_LOG | sed 's/[^m]*m//g;s/starting//')
			RESULT=$(grep 'FAIL\|Panic\|snapshot test' $PHANTOM_LOG | tr '\n' ';')
			SEND_LOG="$0.log $PHANTOM_LOG"
		fi
		sed 's/[^m]*m//g;s///g' $SEND_LOG | mail -s "$VERSION: ${RESULT:-test ok}" ${MAILTO:-$(whoami)}
	}
}

if [ $# -gt 0 ]
then
	FOREGROUND=-f

	while [ $# -gt 0 ]
	do
		case "$1" in
		-f)	FORCE=1			;;
		-r)	CHECK_GIT=1		;;
		-u*)	UNATTENDED=-unattended	;;
		-p)
			shift
			[ "$1" -gt 0 ] && PASSES="$1"
		;;
		-w)	WARN=-w
			MSG="No updates for today"
		;;
		-nc)	unset COMPILE	;;
		-ng)	TEXTONLY=-ng	;;
		-ns)	unset SNAPTEST	;;
		-nt)	unset TESTRUN	;;
		-v)	VIRTIO=-v	;;
		-x)	DEBUG=-x
			set -x
		;;
		*)
			echo "Usage: $0 [-f] [-r] [-u] [-p N] [-w] [-nc] [-ng] [-ns] [-nt] [-v]
	-f	- force run (even if there is another copy stalled)
	-r	- force repository check (and quit if no updates)
	-u	- run unattended (don't stop on panic for gdb)
	-p N	- make N passes of snapshot test (default: N=2)
	-w	- show make warnings
	-nc	- do not compile (run previously compiled version)
	-nt	- do not run test suite
	-ns	- do not run snapshot test (shortcut for -p 0)
	-ng	- do not show qemu/kvm window
	-v	- use virtio for snaps
	-x	- turn on debug output
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

cd $PHANTOM_HOME

preserve_log ( ) {
	[ -d ../public_html/. ] || return

	SAVED_LOG=../public_html/$1

	C=9

	while [ $C -gt 0 ]
	do
		T=$(expr $C - 1)

		[ -e $SAVED_LOG.$T ] && mv $SAVED_LOG.$T $SAVED_LOG.$C
		C=$T
	done

	[ -e $SAVED_LOG ] && mv $SAVED_LOG $SAVED_LOG.0

	cp $TEST_DIR/$1 $SAVED_LOG

	echo "The $1 can be viewed at

http://misc.dz.ru/~$(whoami)/$1

Previous copies are kept ($1.0 through .9)"
}

# update data BEFORE checking for stalled copies
[ "$CHECK_GIT" ] && {
	rm -f make.log
	GIT_OUT=$(git pull)
	[ $? -ne 0 -o $(echo "$GIT_OUT" | grep -c '^Already up-to-date') -ne 0 ] && \
		die "$MSG"

	echo "$GIT_OUT"
}


# check if another copy is running
[ "$FORCE" ] || {
	RUNNING=$(ps xjf | grep $ME | grep -vw "grep\\|$$")
	DEAD=$(ps xjf | grep $QEMU | grep -vw "grep")
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
		call_gdb $GDB_PORT $(echo "$DEAD" | awk '{ print $2 }')

		preserve_log serial0.log
	}
#[ -s $0.lock ] && exit 0
#touch $0.lock
	IN_USE=$(netstat -ntpl4 2>/dev/null)

	while [ $(echo "$IN_USE" | grep :$GDB_PORT) ]
	do
		echo "Somebody took my gdb port! Taking next one..."
		GDB_PORT=$(expr $GDB_PORT + 1)
		[ $GDB_PORT -gt $GDB_PORT_LIMIT ] && {
			echo "Too many attempts. Aborted"
			exit 0
		}
	done
}

# now it is safe to alter behaviour on exit
trap at_exit 0 2

[ "$COMPILE" ] && ./ci-build.sh $FOREGROUND $WARN $DEBUG
[ $? -eq 0 -a "$TESTRUN" ] && ./ci-runtest.sh $FOREGROUND $UNATTENDED $TEXTONLY $DEBUG
[ $? -eq 0 -a "$SNAPTEST" ] && ./ci-snaptest.sh $FOREGROUND $UNATTENDED $VIRTIO $TEXTONLY ${PASSES:+-p $PASSES} $DEBUG
