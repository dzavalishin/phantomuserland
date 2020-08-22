## IDE
  C programming (kernel) - Visual Studio Code (supports win/mac/linux)
  Java programming - Eclipse


## Linux (tested on ubuntu 19.x):
  gcc

  apt get install:

    - gcc-multilib
    - libcunit1 libcunit1-dev
    - openjdk-8-jdk

  for working on ARM port:

    - gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

## Windows:
  X86 - Cygwin gcc. (See etc/cygwin_get.cmd)
  Possibly, mingw will do, but nobody tried. (see etc/cygwin_get.cmd)

  MIPS/ARM - <http://opensource.zylin.com/gccbinary.html>
  MIPS little endian - <http://code.google.com/p/uos-embedded/wiki/gcc_mipsel_ru>

    NB! Current MIPS kernel is big endian.

## Qemu
  Linux   - etc/qemu_0.15.0-2_amd64.deb package for instalation of valid qemu version

  Windows - <http://lassauge.free.fr/qemu/>

  MIPS info (russian) <http://code.google.com/p/vak-opensource/wiki/qemu_mips_machines>

  OpenVPN for Win - <http://openvpn.net/index.php/open-source/downloads.html>
