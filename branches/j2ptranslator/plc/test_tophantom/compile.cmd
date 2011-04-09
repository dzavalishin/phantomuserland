javac b.java
mkdir translation\java\test
copy .\b.class .\translation\java\test
javac -cp . a.java
copy .\a.class .\translation\java\test
javac -cp . field.java
copy .\field.class .\translation\java\test
javac -cp . obj_test.java

javac compare.java
javac loop.java
javac logic.java
javac array.java
javac switch_test.java



