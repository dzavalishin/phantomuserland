OUTPUT=mkpfs.jar
CLASS=ru.dz.phantom.pfsformat.Main

all:: jar

jar:: $(OUTPUT)

install:: $(OUTPUT)
	cp --target-directory=../../build/jar $(OUTPUT)

$(OUTPUT): bin/ru/dz/phantom/pfsformat/*.class bin/ru/dz/phantom/disk/*.class
	cd bin ; jar cvfe ../$(OUTPUT) $(CLASS) .

clean::
	-rm -rf bin/*
