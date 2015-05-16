# Brief #

**There is a persistent list which keeps references to ALL objects kernel is able to access. To make sure GC does not kill 'em and to process restart. It is called "restart list".**

**When object world needs kernel to access some object, it requests putting object to restart list. Restart list index is used by kernel to access object, no real object address (ref) is kept in kernel data structures.**

**On kernel boot restart list is rebuilt. Old one is moved off and new one initialized. Then old one is processed one item at a time - if item class is internal, special restart C function is called. After that reference is deleted (refcmt decremented), so if that is the last ref, object will die. Restart function can request putting object in new restart list, re-establishing kernel connection somehow, and in this case object will continue to exist.**

**Object can be removed from restart list manually, but serious care must be taken to ensure that no one will attempt to access it from kernel after that.**

# Q #

**Can restart list be compacted? Id reuse?**


# Details #

**pvm\_add\_object\_to\_restart\_list must return index (handle)** need f to map handle to ref