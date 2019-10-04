OUTPUT=pfsextract.jar
CLASS=ru.dz.phantom.pfsextract.Main

all:: jar

jar:: $(OUTPUT)

install:: $(OUTPUT)
	cp --target-directory=../../build/jar $(OUTPUT)

$(OUTPUT): bin/ru/dz/phantom/pfsextract/*.class bin/ru/dz/phantom/disk/*.class
	cd bin ; jar cvfe ../$(OUTPUT) $(CLASS) .

clean::
	-rm -rf bin/*
