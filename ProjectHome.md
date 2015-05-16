<img src='http://dz.ru/solutions/phantom/images/phantom_mask_small.png' align='right'>
<h1>Welcome</h1>

This is the Phantom OS project. <br><br>

Phantom is an attempt to forget about how it used to be and design operating system from scratch, building a small simple and othrogonal subset of services, which will make programmer's life really easier.<br>
<br>
<ul><li>OS shutdown is invisible to user code - just as if nothing happened at all.<br>
</li><li>No REQUIREMENT to use files - everything is persistent, so program state is not required to be serialized anymore.<br>
</li><li>No separate address space walls between everything, but EVERY single object is protected from other's object code.<br>
</li><li>Very efficient and easy high-level IPC (Access anything you have a pointer to. No, you can't create a pointer yourself - just receive it from outside.)<br>
</li><li>OS provides a lot of modern high-level services such as SOAP, XML RPC, general conversion of object tree to XML and vice versa.<br>
</li><li>No process, just threads.<br>
</li><li>More. See some article <a href='http://groups.google.com/group/phantom-os/web/instead-of-the-manifest?hl=ru'>on Google group</a>.</li></ul>

<h2>What Phantom is not</h2>

We are not one more Linux clone. We are not one more microkernel per se. We're not "look dad, I have protected mode code working". We're after a KISS-principle based ortogonally-persistent environment, which will let programmers to work effectively.<br>
<br>
<h2>How to run current kernel</h2>

NB! Don't expect too much.<br>
<br>
<ul><li>Download runPhantom.zip and latest classes.rar & phantom.rar.<br>
</li><li>unpack all of them<br>
</li><li>put unrared phantom and classes files to runPhantom/tftp subdir<br>
</li><li>run zero_ph_img.cmd and phantom.cmd then - QEMU wiil start</li></ul>


<h2>How to contact us</h2>

If you want to take part in Phantom OS development, please contact Dmitry Zavalishin, dz@dz.ru. Add 'phantom' word to subject line, please!<br>
<br>
See HowToTakePart.<br>
<br>
<br>
<h2>Project Overview and Road Map</h2>

Read <a href='http://code.google.com/p/phantomuserland/source/browse/#svn/trunk/doc'>documents</a>.<br>
See RoadMap.<br>
<br>
<br>
<h2>Project Progress</h2>
<b>Oct 2011</b>: ARM port is more stable now, can go on with drivers.<br>
<br>
<b>Sem 2011</b>: Kernel profiler, some optimizations, ACPI lib import, graphical code progress.<br>
<br>
<b>Aug 2011</b>: Procfs gives out stats and threads. Most of asm code for MIPS port is written.<br>
<br>
<b>Jul 2011</b>: Disk IO cache for filesystems. Snapshot via new io stack (with partitions). Posix: cd/getcwd<br>
<br>
<b>Jun 2011</b>: Generalized kernel object pool. Usrland POSIX regression tests. AHCI and USB drivers started. Floppy driver, new disk IO stack refined.<br>
<br>
<b>May 2011</b>: Remote object debugger.<br>
<br>
<b>Apr 2011</b>: First steps of mips/amd64 ports. 64 bit cleanup. Progress on Java2Phantom - switch.<br>
<br>
<b>Mar 2011</b>: Code divided in portable and machdep parts. Arm port started. SNTP.<br>
<br>
<b>Feb 2011</b>: Regression tests again, refinement of Unix processes code.<br>
<br>
<b>Dec 2010</b>: Some progress on Java bytecode translator, more of regression tests.<br>
<br>
<b>Nov 2010</b>: TCP, tiny ftpd, some reflections support in class files.<br>
<br>
<b>Oct 2010</b>: Debugging tools, kernel events statistics.<br>
<br>
<b>Sep 2010</b>: External fsck. BeOS/Haiku messages/ports.<br>
<br>
<b>Aug 2010</b>: Phys mem page reclaim code.<br>
<br>
<b>Jul 2010</b>: SMP support is basically implemented, but needs debugging.<br>
<br>
<b>Jun 2010</b>: Snapshot engine is back!<br>
<br>
<b>May 2010</b>: Running on real hardware for the first time!<br>
<br>
<b>Apr 2010</b>: UI events, Z Buffer.<br>
<br>
<b>Feb 2010</b>: Unix emulation started, first test usermode 'unix'-like thread is running.<br>
<br>
<b>Jan 2010</b>: Trap/IDT, using elf symtab in stack dump.<br>
<br>
<b>Dec 2009</b>: License clean (own, in short:) multithreading/scheduling code.<br>
<br>
<b>Nov 2009</b>: First ARP packet is received and processed. UDP send seems to be ok as well.<br>
<br>
<b>Oct 2009</b>: We have Java bytecode (including method call) compiled to Phantom bytecode. Compiler is very far from being complete, but first steps are done.<br>
<br>
<b>Sep 2009</b>: Preliminary (but stable) GC is implemented. First time app code was able to work untill killed. :)<br>
<br>
<b>Aug 2009</b>: First working Phantom kernel and userland program (which is Tetris:) was demonstrated at Chaos Constructions computer festival in S. Petersbourg, Russia.<br>
