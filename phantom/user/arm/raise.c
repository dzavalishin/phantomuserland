#warning implement me right

// /usr/lib/gcc-cross/arm-linux-gnueabihf/4.8/sf/libgcc.a(_dvmd_lnx.o): In function `__aeabi_ldiv0':
// (.text+0x6): undefined reference to `raise'

void raise()
{
    exit(33);
}
