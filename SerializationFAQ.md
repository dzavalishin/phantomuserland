Q: **I was told or decided by myself that Phantom idea is nothign more than serialization, and serialization exists in any decent language/environment. So Phantom is pointless**.

A: **Is serialization effective? No.** It takes quite a lot of computer resources. So if we can skip it, we gain.

A: **Is DEserialization effective? No.** The most widely used container is XML stream, and XML parsing is really not free. And, BTW, you can not extract one element from XML stream, modify it and put back. Well, you can, but you will have to process all the XML for that.

A: **Is it possible to serialize ANY object of ANY class? No.** Any serialization technique brings its own limitations, which make it impossible to use serilization to process set of arbitrary objects.

A: **Is it possible to serialize all the program state at once? Usually - no.** Due to the previous limitation.

A: **Is it easy to serialize part of program's state? No, it is not really easy.** Though some serialization engines provide instruments to select objects to be saved, it is a real pain in the butt to cut and recreate connections between saved and unsaved part of program state.

A: **Is it possible to have two interconnected sets of objects serialized separately? Can we recreate them so that they still will be interconnected? Oh, God...** Well, theoretically it is possible, but in reality i'd better shoot myself in a foot instead.

Serialization is a hack we have to use in absence of persistence. It does not solve all the problems. It's just a temporary and partial solution.


Q: **But it is obvious that Phantom will need serialization too, right?**

A: **Yes and no.** Yes for it is a natural way to communicate with legacy systems. But the difference is that in current OSes you HAVE to serialize, want you, or nat. In Phantom you're ABLE to do that, IF you really need and WHEN you really need.

Q: **Will Phantom have default serialization engine?**

A: **Of course.** Currently we think about XML serialization tools akin to Java XMLEncoder/XMLDecoder.