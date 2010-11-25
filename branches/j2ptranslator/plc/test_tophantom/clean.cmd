rmdir /S /Q translation

mkdir tmp
copy .\*.java .\tmp
rem cp ./*.cmd ./tmp
rem cp ./*.pc ./tmp
del a.*
del b.*
del compare.*
del field.*
del loop.*
del logic.*
del array.*
del obj_test.*
del translation.*.*
del translation_*.*
copy .\tmp\*.java .
rmdir /S /Q tmp
