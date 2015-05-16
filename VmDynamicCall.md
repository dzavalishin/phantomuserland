# Introduction #

To support dynamic languages like Pyton and ObjectiveC dynamic (by name) method call must be implemented. It is supposed to be implemented as method call instruction with the instruction format identical to usual method call (inst code byte + 32 bit int), but integer is index into class const table (which is to be implemented as well).

Referenced constant is string whit a method or class.metod name. During execution real method is found and it's direct call instruction is pathced into the bytecode. If string contained just method name, lookup is done in the class pointed by passed 'this'. If global name is given, it is supposed that we do function call and 'this' is, possibly, zero. Class is being looked up (VmClassLookup) as well as method, and referenced constant is replaced with class+method constant used to process a call without lookup the next time instruction is called. No bytecode pathing is done in this case.

To be implemented.

Comments?