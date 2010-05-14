This archive contains `QEMU' version 0.10.0 binaries for the 32-bit Windows
target.

You also need fmod.dll and SDL.dll, which are available as separate downloads
from <http://www.bttr-software.de/qemu/>.

Modifications applied:
  - reverse SDL and VNC window caption
  - add icon and VERSIONINFO resources to EXE file
  - add SDL window icon
  - use wspiapi.h for Windows 2000 compatibility (IPv6)
  - fix VGA issue introduced by r6349

MinGW tools used:
  - MSYS-1.0.10.exe
  - msysDTK-1.0.1.exe
  - binutils-2.17.50-20060824-1.tar.gz
  - gcc-part-core-4.3.0-20080502-2-mingw32-alpha-bin.tar.gz
  - gcc-part-c++-4.3.0-20080502-2-mingw32-alpha-bin.tar.gz
  - mingw32-make-3.81-20080326-2.tar.gz
  - mingw-runtime-3.14.tar.gz
  - mingw-utils-0.3.tar.gz
  - w32api-3.13-mingw32-dev.tar.gz

  Robert Riebisch <rr@bttr-software.de>
