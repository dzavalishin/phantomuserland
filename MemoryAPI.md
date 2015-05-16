## Implemented ##

  * VM 'new' instruction.


## Proposed calls ##

  * Device-specific alloc - lets choose specific hardware paging device to allocate at.
  * Page-aligned alloc - lets allocate binary object, which is completely (including its allocator/object headers) page-alligned and has (total) size mesured in pages. Such objects can be used to build hi performance database-like structures.
  * Page-aligned data alloc - allocates binary object, which DATA area is page-aligned. Such objects can be used to build POSIX/LLVM execution environment.

## Proposed data types ##

  * need some data type to represent paging device.