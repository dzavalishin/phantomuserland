#!/bin/sh
cov-build --dir cov-int sh -c make -k -j 1
#scan-build -o etc/clang-analyzer/ make
