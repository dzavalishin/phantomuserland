rem echo replace CC in Makeconf -> export CC	= /usr/share/clang-analyzer/scan-build/ccc-analyzer 
rem sh analyze.sh

make clean
SET PATH=G:\tools\cov-analysis-win64-7.7.0.4\bin;C:\projects\tools\cov-analysis-win32-7.7.0.4\bin;%path%
SET PHANTOM_NO_PVM_TEST=true
rem cov-build --dir cov-int make -k -j 1
sh analyze.sh
