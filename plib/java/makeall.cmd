call jc src/java/lang/Class.java
call jc src/java/lang/String.java
call jc src/java/lang/Throwable.java 
call jc src/java/lang/InterruptedException.java 
call jc src/test/toPhantom/Assigns.java

call jpc -cbin;class java.lang.Class
call jpc -cbin;class java.lang.String
call jpc -cbin;class java.lang.Throwable
call jpc -cbin;class java.lang.InterruptedException

call jpc -cbin;class test.toPhantom.Assigns
