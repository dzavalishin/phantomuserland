This is Phantom OS test environment. Don't expect too much - current
release is not even at alpha level. It is not supposed to be usable in
any way. :)

To run kernel first time:

1. Run zero_ph_img.cmd first, it will create required disk images
2. Run phantom.cmd - QEMU qill start and boot default kernel

What is included here is kernel and simple userland code which starts
and runs Tetris-like thing forever (forever == some two days, I think).

This demo has SNAPSHOTS TURNED OFF, so no persistence, sorry.

You can go to http://code.google.com/p/phantomuserland/, download
userland environment and try to write some app for this kernel,
but it is no easy task. Though if you wanna try, let me know.

Don't hesitate to contact me: dz@dz.ru, add 'phantom' word to subject.
