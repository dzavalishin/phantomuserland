## Implemented ##

**.phantom.os.time** class

## Proposed calls ##

  * **sleepMsec(n)** - sleep for n milliseconds (implemented, but is not brought to this interface yet)
  * **waitForSnapShot()** - return after next snapshot - make sure that all previous op data on disk.
  * **getCurrentTime()** - must return some 64 bit time? NewOS format?
  * **adjustTime(?)** - paramteter?

## Proposed data types ##

  * 64 bit Time type - usecs since Jan 1, 1 AD
  * 32 bit UnixTime32 - old Unix time