Thank you for testing this modified version of NewOS!

NewOS (http://www.newos.org/) is a modular kernel designed for high portability primarily written by former Be-employee Travis Geiselbrecht. This readme file is accompanied with an image file of a version of NewOS that is in several places modified by me. The most important and visible changes are listed here:

- a proof-of-concept graphical user interface (GUI) with some very refined algorithms
- two simple GUI programs (terminal and lines)
- a completley new network memory managment algorithm (cbuf code) that is more than 800% times faster than the original code
- a VESA VBE 3.0 video card driver that even resolves the most frequent vendor specific VBE 3.0 flaws and plain errors
- a NE2000 and compatible ISA and PCI network card driver
- support for outgoing ICMP messages and the ping utility
- the PCI module re-written
- improved timer code
- vastly improved POSIX support
- port of the bash shell

You can test this image either on real hardware or within an emulator.

If you are out for testing on bare hardware you'll need
- a Pentium or compatible computer with a 3.5" floppy drive
- a VESA VBE 2.0 or VBE 3.0 compliant video card for the GUI
- an empty 1.44 MB floppy disk
- some utility for writing raw images to floppy disks (e.g. Rawrite+ which you can download from my homepage at http://notion.muelln-kommune.net/projects_frame.html#tools, or "dd" on Unix systems)

Put an empty floppy disk into your floppy disk drive, invoke rawrite and enter image file name and floppy drive letter to write the image upon it (our use any other utility to write raw images). Then restart your computer to boot from floppy.

If you are less adventureous (or lack time/floppy disk/drive...), you can test the image within an emulator like QEMU. All you need is to
- download QEMU from http://fabrice.bellard.free.fr/qemu/ (there's a link to a Windows version, too)
- invoke QEMU via "qemu -L . -fda newos-notion.img -net nic,model=ne2k_pci"

You will get some simple instructions on how to access your network, once you boot into NewOS.

If you run NewOS within QEMU and want to be able to access it from outside (ping, telnet), you must use a TAP adapter. Add "-net tap,ifname=OpenVPN" to the QEMU command line (replace OpenVPN with whatever name your TAP device was given).

If you don't have a TAP adapter installed, you're probably under Windows. There's a nice tutorial that explains how to install one at http://www.h7.dion.ne.jp/~qemu-win/TapWin32-en.html. Once you've done that, you might want to activate Internet Connection Sharing in your normal connection's settings. For this to work you MUST set your TAP device's address to 192.168.0.1.

I'm happy to get feedback at notion@notion.muelln-kommune.net. Thanks for testing once again,

Michael Noisternig, student of computer science and mathematics
notion@notion.muelln-kommune.net
http://notion.muelln-kommune.net/
April 6, 2007
