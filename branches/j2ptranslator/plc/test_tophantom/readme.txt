Regression test for Java to Phantom bytecode translator


In this folder source files:
  a.java
  b.java
  obj_test.java
compile to .class files and then translate to pc.files (see all.cmd).

change classpath in translation.cmd if need.


In ./boot folder
  ru.dz.phantom.system.boot.ph
  ru.dz.phantom.system.translation_regression_tests.ph
need compile to .pc files and include to classes for run with pvm_test.exe.
others .ph files in this folder contein some functional as .java files (they used to compare result bytecodes).
include them to classes or comment in ru.dz.phantom.system.translation_regression_tests.ph.
