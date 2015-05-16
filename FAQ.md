(in progress. feel free to add questions)

This is a top-level :) FAQ and list of other FAQ pages.

See:
  * [TechFAQ](TechFAQ.md) for tech details related questions.
  * [GeneralFAQ](GeneralFAQ.md) for more common questions.
  * [SerializationFAQ](SerializationFAQ.md) for questions on serialization.
  * [DoYouKnowFAQ](DoYouKnowFAQ.md) for "_Do you know XXX_" style questions. :)

## Classification ##

<br><br>
Q: <b>Is Phantom a POSIX-compliant system?</b>
<br>
A: Not necessarily. It is possible to layer POSIX subsystem above the Phantom native environment, but it is not an idea per se. Though we'd like to implement POSIX subsystem later.<br>
<br><br>
Q: <b>Is Phantom an object-oriented OS?</b>
<br>
A: Yes. Basically you can speak about Phantom as a huge pool of fine-graied objects.<br>
<br><br>
Q: <b>Is Phantom a microkernel OS?</b>
<br>
A: Yes. It is not enforced (kernel can be extended, if it is needed, and is fully multithreaded), but basic Phantom implementation requires a very small kernel. Generally, kernel contains threading and paging code only. All drivers except for the disk IO can be implemented in userland.<br>
<br><br>
Q: <b>Does Phantom have processes?</b>
<br>
A: Phantom has threads. Process (group of threads, sharing some global state such as current directory and some other historical junk) is not needed in Phantom. But any group of threads can set up their own common object and keep common state in it.<br>
<br><br>
Q: <b>What about separate address spaces?</b>
<br>
A: No. No! At this point you thought to yourself something like “than Phantom can not protect one application from other”, and were wrong. Phantom is one big address space. But, nevertheless, everything inside is protected. Protection is based on a simple idea. Phantom is a big virtual machine. And this VM has no means to convert integer to pointer – due to this it is impossible to scan through address space and gan access to anything you have no pointer to. That simple. And – yes, due to the absence of separate address spaces IPCs are really cheap in Phantom. And there are no context switches, which adds to the system effectiveness. One can argue that VM makes system to run slowly, but nowadays this problem is solved with effective JIT compilers, so we don’t expect real degradation due to the VM. Moreover, result of JIT compilation can be stored so usual Java-like startup penalty won’t exist in Phantom either.<br>
<br><br>
Q: <b>File system?</b>
<br>
A: Nope. Sorry. Nobody needs files in Phantom. All the operating system state is saved across shutdowns. Phantom is the only global persistent OS in the world, AFAIK. All the state of all the objects is saved. Even power failure is not a problem, because of unique Phantom’s ability to store its’ complete state on disk frequently. The most unusual Phantom property is its hybrid paging/persistence system. All the userland memory is mapped to disk and is frequently snapped. Snapshot logic is tied with the common paging logis so that snapshots are done cheap way. From the application’s point of view it means that all the user documents or any othet program state doesn’t have to be squished into the linear filespace with the help of the serialization code, as it is in classic operating systems. Anything is kept in its internal, “graph of objects” form. This means that Phantom programs are much simpler and more efficient too. Opening text document in classic OS means reading file (transferring its data to specific place in process’s memory) and then converting its contents to program’s internal form (decoding and once more moving data), and just then – showing it to user. Opening text document in Phantom means just executing some object’s printMe() method – all the data is ready and available directly without conversion.<br>
<br><br>
Q: <b>So what happens when Phantom reboots?</b>
<br>
A: You will get your desktop in the same state as it was before shutdown. Everything intact.<br>
<br><br>
Q: <b>And if I had some, say, ssh or telnet connection?</b>
<br>
A: It will die. Sorry. Of course, telnet app can restart connection for you – if some application needs to be informed about system restart, Phantom will provide special signalling. But most applications won’t even notice that shutdown happened.<br>
<br><br>
Q: <b>OS is based on VM – does it mean that not all the possible programming languages will be supported?</b>
<br>
A: Yes. Say goodbye to C and Assembler. On the other side, everything is in Java or C# now, or even in some even more dynamic language, such as Javascript or, even, PHP. All these languages will be supported.<br>
<br><br>
Q: <b>So Phantom has no address arithmetics?</b>
<br>
A: It has, but only inside the single object. And objects can be huge. No limit.<br>
<br><br>
Q: <b>Is it a 32 or 64 bit OS?</b>
<br>
A: It is meaningless to have 32 bit Phantom, because of single address space. We are going to have 64-bit version as mainstreem one and the existing 32-bit implementation will be, possibly, left for small handheld devices.<br>
<br>

<p>
Q: How can app live without files?<br>
<br>
A:<br>
<br>
<img src='http://misc.dz.ru/~dz/phantom/Application_Data_Flow.png' />