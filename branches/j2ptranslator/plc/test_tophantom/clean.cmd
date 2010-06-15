rmdir /S /Q translation

mkdir tmp
cp ./*.java ./tmp
rem cp ./*.cmd ./tmp
rem cp ./*.pc ./tmp
del a.*
del b.*
del compare.*
del field.*
del loop.*
del obj_test.*
del translation.*.*
del translation_*.*
cp ./tmp/*.java .
rmdir /S /Q tmp
