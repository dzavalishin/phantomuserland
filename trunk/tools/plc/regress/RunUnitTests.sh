#!/bin/sh


function do_real_java() {
    fileName=$1;
    className=${fileName/.java/};
# -Xlint:-options disables bootstrap class path warning. or use --bootclasspath 
    echo '     ---- Real Java run';
    javac -source 1.4 -Xlint:-options $fileName;
    java $className < input > $className.realOutput;
}

function java_to_phantom() {
    fileName=$1;
    className=${fileName/.java/};


    echo '     ---- Make Phantom class files';

    set -vx
#    java -cp $PHANTOM_HOME/tools/plc/lib/soot-2.5.0.jar -jar $PHANTOM_HOME/build/jar/jpc.jar -o$PHANTOM_HOME/plib/bin -I$PHANTOM_HOME/plib/bin $className
    java -cp $PHANTOM_HOME/tools/plc/lib/soot-2.5.0.jar -jar $PHANTOM_HOME/build/jar/jpc.jar $className
    set +vx
}

function mytest() {
    echo ' ---- ' $1;

    do_real_java $1 ;
    java_to_phantom $1 ;
}

function _mytest(){

    echo 'starting function test';
    i=$1;
    j=${i/.java/};
    if java -Xmx400M soot.Main -f J -src-prec java -app $j > $j.produceJimple;
    then
        echo $i 'Passed Producing Jimple';
        if java -Xmx400M ru.dz.soot.SootMain -src-prec java -app $j > $j.produceClass;
        then
            echo $i 'Passed Producing Class';
            javac -source 1.4 $i;
            java $j < input > $j.realOutput;
            if java -cp sootOutput $j < input > $j.myOutput;
            then
                echo $i 'Passed Running';
                if diff $j.realOutput $j.myOutput > $j.diff;
                then
                    echo $i 'Passed Diff';
                else
                    echo $i 'Failed Diff';
                fi
                rm $j.diff;
            else
                echo $i 'Failed Running';
            fi
            rm $j.myOutput;
            rm $j.realOutput;
        else
            echo $i 'Failed Producing Class';
        fi
        rm $j.produceClass;
    else
        echo $i 'Failed';
    fi
    rm $j.produceJimple;

}

echo 'starting regression tests';

cp ../../../plib/bin/*.pc test/pc

listf=RunJpcUnitTests.tmp
listj=RunJpcUnitTests.java
#listj=${listf/.tmp/.java/};
echo 'public class RunJpcUnitTests {   ' > $listf
#echo '	public void say( String s ) { System.out.println(s); } ' >> $listf
echo '	public void say( String s ) {  } ' >> $listf
echo '' >> $listf
echo '	public void run() { String [] args  = new String [0]();  ' >> $listf
echo '' >> $listf
rm $listj



for i in *.java;
do
    fileName=$i;
    className=${fileName/.java/};
    mytest $i ;
    echo '	say("--- DO REGRESS ---' $className '");' $className'.main(args);' >> $listf 
done

echo '}}' >> $listf 
mv $listf $listj
java_to_phantom $listj ;

