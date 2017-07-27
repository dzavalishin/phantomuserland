all:: jar

jar:: mkbulk.jar

install:: mkbulk.jar
	cp mkbulk.jar ../../build/jar

mkbulk.jar: bin/ru/dz/phantom/mkbulk/*.class
	cd bin ; jar cvfe ../mkbulk.jar ru.dz.phantom.mkbulk.Main .

clean::
	-rm -rf bin/*
