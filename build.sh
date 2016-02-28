#! /bin/bash
#
# you must stand in phantom home when you run this script
#
export PHANTOM_NO_PVM_TEST=true
export PHANTOM_HOME="`pwd`"
[ -L phantom/kernel ] || ln -s ../oldtree/kernel/phantom phantom/kernel
make -C phantom "$@"
make -C oldtree/kernel/phantom "$@"
