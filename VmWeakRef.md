# General idea #

Weak reference object is exactly one per real object and is shared. (So it must be supported in compiler? Or in create object code? If using satellite ref there can be any count of weak refs)

Each object has reference to its weak ref object (counted by GC), and weak ref object has hidden (not counted by GC) pointer to its real object.

Performance note: Object having weak ref to itself must be marked by runtime flag.

On refcount to zero or some else GC decision to delete referenced object, GC checks if weak ref exists (by testing abovesaid flag) and if so, calls weakref clean function, which is in mutex with weakref 'get reference' method. This function either cleans weakref and returns 0 (weakref was idle and didnt change refcount), which means weakref is clean now GC can really delete object, or 1 (just during the call weakref produced new reference and, so, refcount is not zero, which means GC should back off.

Weakref 'get reference' method, in turn, is being interlocked with clean function and increments object use count in mutex.

# Satellites chain #

Any object can have any number of chained related objects called satellites. They're pointed by _satellites field of object header. Weak refs are such satellites._

Q: chained? Or just use array so that _satellites points to array, and it has all of satellites? Or even point_satellites to satellite as long as we have just one?

Other possible satellites are:

  * mutex/sema object for implementing java-like sync primitives;
  * container object to hold data invisibly attached by some user/class
  * owner or rights