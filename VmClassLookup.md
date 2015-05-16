# Introduction #

Getting pointer to class.class object bt class name is one of the basic requirtements of the VM environment. This includes ability to find a class implementation over the network as well.

Currently kernel uses user-level class called directly by kernel to find class implementation. It is supposed to be prepended by fast kernel-level hash or tree lookup, but in any case to find class for the first time user code is called. This way class lookup can be exteded and modified without patching or updating kernel.

The only way to get class that exists currently and is available to user class lookup code is kernel classloader which is able to pull class by name from OS distribution bundle using some kernel level black magic.

Later network class lookup is supposed to be added so that kernel class bundle will need to have only minimal environment needed to start netwok access and continue to run loading classes from net.

# Questions #

Class signing?