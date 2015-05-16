## Accessor ##

**.phantom.os** - main accessor class

## Existing APIs ##

  * **[TimeAPI](TimeAPI.md)** - time related calls (_getTimeServer()_)

## Todo ##

  * Threads (have wild impl, needs to be wrapped)
  * Net IO (TCP,UDP)
  * Windows (started, not done)
  * Random (Have pseudo-random in tetris code, extract, seed, need kernel API)
  * Environment (name=value, have some impl. in .phantom.environment, need to define keys, access, etc)
  * TTY style windows (have wild impl., need wrapper or redesign)
  * [ShellAPI](ShellAPI.md) - object directories, post boxes (to pass objects to another user), etc
  * Application - base class for app
  * Startup shell class control - some way to put kernel command line to kernel env, choose root shell class by kernel env. field value
  * **[MemoryAPI](MemoryAPI.md)** - special memory allocation related calls (_getMemoryServer()_)