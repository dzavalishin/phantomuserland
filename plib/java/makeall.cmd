@sh makeall 

rem call jc src/java/lang/Class.java
rem call jc src/java/lang/String.java
rem call jc src/java/lang/Throwable.java 
rem call jc src/java/lang/InterruptedException.java 
rem call jc src/java/lang/CloneNotSupportedException.java 

rem call jc src/test/toPhantom/Assigns.java

rem call jpc -cbin;class java.lang.Class
rem call jpc -cbin;class java.lang.String
rem call jpc -cbin;class java.lang.Throwable
rem call jpc -cbin;class java.lang.InterruptedException
rem call jpc -cbin;class java.lang.CloneNotSupportedException

rem call jpc -cbin;class test.toPhantom.Assigns

