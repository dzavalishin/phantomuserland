javac b.java
mkdir translation\java\test
cp ./b.class ./translation/java/test
javac -cp . a.java
cp ./a.class ./translation/java/test
javac -cp . field.java
cp ./field.class ./translation/java/test
javac -cp . obj_test.java

javac compare.java
javac loop.java
javac logic.java


