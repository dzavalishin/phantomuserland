# Nearest Goal #

To produce distributable alpha-level OS which can be programmed for by 3rd party programmers. **We're quite close to this.**


## Userland/Tools Tasks ##

Quite crirtical tasks.

|**Java bytecode compiler**. Have skeleton, need complete one. At least to the level where ciontainers and UI code is converted.|**Really urgent.**|
|:------------------------------------------------------------------------------------------------------------------------------|:-----------------|
|**Hosted test env. multithreading**. Port existing threads lib to work in unix userland/cygwin.|Must be easy.|
|**Hosted test env. TCP access**.|Ditto.|
|Windowing events, input, event thread pools. Need userland iface. Need code cleanup.|**In progress**.|
|**VM and compiler support for floats/doubles/64 bit ints**. Should be really easy.|- |
|**More of Phantom language implementation in compiler**. Closures, direct containers support.|- |
|**VmWeakRef** - stuck with synchronization!|**In progress**|
|**Border objects and restart exceptions**. Especially for TCP interfaces. Depends on weakrefs!|**Nearly done**|
|Windowing env Z-buffer|**Done**.|

## Kernel Tasks ##

|**VirtIO drivers**. Have incomplete disk driver, net driver in progress.|Finish!|
|:-----------------------------------------------------------------------|:------|
|**IDE driver cleanup**. Existing driver is very strange.|**started**|
|**IP stack tests and userland interface**.|- |
|**ARM port**. Machdep code and some drivers are done.|In progress|
|**Partition lookup code**. |PC partitions implemented. EFI?|
|**Disk selection**. Right now Phantom looks for its disk in hardcoded places. Partitioning system reckognizes Phantom partitions and disks. Has to be connected to pager.|Done.|

## Far Goals ##

|**Graphical shell**. Something to start with.|Nope.|
|:--------------------------------------------|:----|
|**Posix support**. Map two binary objects to DS/CS, run native thread, support syscall mapping.|**Partially working.**|
|**LLVM fast code**. In addition to posix subsystem it would be nice to have a specific LLVM subsystem which can offer programmers hispeed, but portable (to Phantom on other arch) programming framework. The same CS/DS binary objects, but executable code is LLVM-level and passed through LLVM backend before being run for the first time.|Nope.|
|**OpenGL**. In fact, it is quite easy to get TinyGL working (it is already done for kernel), but we need a clean userland interface and understanding of interoperation for multiiple simult. threads doing GL rendering in a same env.|Just an userland lib?|
|**Sound drivers**. At least some very simple AC97 wave output one?|**In progress**|
|**Video codecs environment**. Seems to be requiring Posix/LLVM support first. The first goal is to be able to give it an MPEG binary for playing.|**Quite near now**|


# General Project Support Tasks #

  * **Doxygen**. And code documentation too.
  * **Test suite**, regular regression test setup. **mostly ok**
  * **Code review**. Need code review process to be set up and carried.
  * **Good internal document(s) on development process**. For new members to jump in easily.
  * **Define branching/merging rules**. We will have to have release branches.


# Later... #

  * **Python compiler**. Seems to be a good possible object env shell and common scripting language. Or shall it be Ruby instead?