CPATH="./bin;junit/junit.jar"
#CDB="-verbose:class"
java $CDB -cp $CPATH org.junit.runner.JUnitCore ru.dz.phantom.ext_tests.UdpEchoTest
java $CDB -cp $CPATH org.junit.runner.JUnitCore ru.dz.phantom.ext_tests.TcpEchoTest
