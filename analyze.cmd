echo replace CC in Makeconf -> export CC	= /usr/share/clang-analyzer/scan-build/ccc-analyzer 
rem sh analyze.sh

SET PATH=C:\projects\tools\cov-analysis-win32-7.7.0.4\bin;%path%
cov-build --dir cov-int make -j 1
tar czvf phantom-cov.tgz cov-int

