#! /bin/bash
#
# you must stand in phantom home when you run this script
#
export PHANTOM_NO_PVM_TEST=true
export PHANTOM_HOME=$(pwd)

# moved to config.mk
#if [ "`uname`" == "Darwin" ]
#then
#	export TARGET_OS_MAC=1
#	export CC_DIR=/usr/local/Cellar/llvm/4.0.1/bin
#fi

[ -L phantom/kernel ] || ln -s ../oldtree/kernel/phantom phantom/kernel
make -C phantom "$@"
make -C oldtree/kernel/phantom "$@"
