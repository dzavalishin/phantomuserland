(in progress. feel free to add questions)

This is a technological (programmers') FAQ. See [FAQ](FAQ.md) for others.

## Tech ##


Q: **How do you change program code, if program data is just object state?**
<br>
A: It will be possible to uprgade class code as long as class structure is the same. See PhantomClassVersion.<br>
<br>
A: If nem implementstion is very different, user will have to do document conversion, as he does today with conventional software. Or it can be done automatically, by using new class's copy constructor.<br>
<br>
<br>
Q: <b>I have been told that the Phantom will be working very slowly, because it is based on interpreted code.</b><br>
A: 90% of today's professional software is based on «interpreted» code. That includes programs in Java, C#, PHP, Python, Ryby programming languages – in short, all modern ones. This does not mean that such programs work slowly. Compilation of such programs in traditional systems is being implemented directly during the execution. This approach slightly slows launch of the program; but otherwise performance is not slower, and sometimes (in the case of Java and C#) superseeds performance of programs written in obsolete languages (C++, Pascal, etc). In the Phantom OS delay in launching the program is eliminated as a result of JIT-compilation system remaining for reuse. In addition, because of the lack of context switching between kernel and application Phantom will provide greater overall performance of real code, than for example classic Unix.<br>
<br>
See also: <a href='http://ru.wikipedia.org/wiki/JIT'>http://ru.wikipedia.org/wiki/JIT</a>


Q: <b>Is JIT already implemented in OS Phantom?</b>
<br>
A: No. We are not confident that the current format and semantics of the kernel’s bytecode are final. We will go to work on JIT after semantics will be fixed. Strictly speaking, for the past 4 years of the Phantom it has not been changed, so most likely this issue is almost settled now. Only to check some subtle aspects of the system in the restart phase. To build JIT system, LLVM is considered as one of the options, so that we do not foresee big problems in this part of our job.<br>
<br>
See also: <a href='http://dotgnu.org/'>http://dotgnu.org/</a>

Q: <b>How will you deal with the problem of drivers?</b>
<br>
A: It should be noted that in the last 5 years, this issue has lost its severity. Many drivers exist in the source code; a variety of devices has decreased significantly. For example, today only three video card drivers could cover, perhaps, 95% of computers on the market. And these three drivers may be transferred from Linux. Many peripheral devices nowaday connect via USB or FireWire, which actually means that they require only two actual low level drivers – for USB and FireWire ports as such. But these ports are implemented in a fairly standard way. So the problem – although still quite serious – ceased to be immense.<br>
<br>
Q: <b>Is there some virtualization support mechanism (for example: qemu + win23/linux), and what is your vision of the future of this technology in Phantom?</b>
<br>
A: It is possible, and there is no reason not to implement it now. However, we rather see the version of the Phantom itself and any other operating system under the general type of hypervisor such as Xen. At the same time, support for Xen in Phantom facilitates its launch on Amazon Cloud.<br>
<br>
Q: <b>They say in the Phantom there are no files – is it true? How do you live without files?</b>
<br>
A: Yes, it's true. The files themselves are only needed for one reason. They keep «souls of dead programs» – states of programs in the interval between the stop and restart of the program. Since program in Phantom does not need to rest (more accurately – to lose internal state, because it could be stopped), there is no reason to write its condition elsewhere.<br>
<br>
Q: <b>What about end-user? Where will he put his Word paper?</b>
<br>
A: You will not notice disappearance of files. For user, there will be folders and icons-documents in them. Icon will activate (conditionally speaking – it will run the editDocument method in new threads) an object, relevant to the user’s document. Just a document will not be stored in serialized «file» form, but exactly as it is present in the program and is convenient for the program to work. This means that 1. programmer no longer be writing code recording the status of the file and reading it back (in different situations, it is from 30 to 70% of program’s code), 2. actual «opening of the document» will be much faster (no de-serialization process, as there is no data moved from place to place) and 3. any document can easily integrate itself into any other document – by simply adding into itself a reference to a foreign object.<br>
<br>
Q: <b>What do I do if I lose part of the disk? In the normal system, I lose some files, and you will loose all at once. Right?</b>
<br>
A: Not really. Objects in the Phantom are marked in such a way that even basing on a «snatch» of memory, they can be restored. The problem lays in identifying situation «snatches» in the address space. We plan to put in memory special facilities beacons with a certain period, which just record their own address. In this way data can be restored from «broken» address space of Phantom in case of fatal disk failure.<br>
<br>
Q: <b>If the program is poorly written and “eats up” memory, we are used to kill her and start anew. And now?</b>
<br>
A: And now. Indeed, it is necessary to distinguish between job of the program and state of the program. Phantom saves the state, thus making unnecessary «manual» saving of it’s state; but it does not require all programs to work continuously. That is, to keep process/thread running.