#! /bin/bash
#
# you must stand in phantom home when you run this script
#
export PHANTOM_HOME=$(pwd)

die ( ) {
#	[ -s make.log ] && tail make.log
	[ "$1" ] && echo "$*"
	exit 0
}


#[ -L phantom/kernel ] || ln -s ../oldtree/kernel/phantom phantom/kernel
make -C phantom "$@" || die "Make libs failure"
make -C oldtree/kernel/phantom "$@" || die "Make kernel failure"
#cd test
#make unit || die "Make unit failure"
#make plc || die "Make PLC failure"
