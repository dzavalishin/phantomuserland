rmdir /S /Q translation

mkdir tmp
cp ./*.java ./tmp
rem cp ./*.cmd ./tmp
rem cp ./*.pc ./tmp
del a.*
del b.*
del obj_test.*
del translation.*.*
del translation_*.*
cp ./tmp/*.java .
rmdir /S /Q tmp
