
#ifndef CIR_DEFS_H
#define CIR_DEFS_H

#include <ia32/pio.h>
#include "video_drv_basic_vga.h"

//#include <pc.h>

typedef enum {
    CLGD5426, //1
    CLGD5428, //1
    CLGD5429, //2
    CLGD5430, //3
    CLGD5434, //4
    CLGD5434E, //5
    CLGD5436, //6
    CLGD5440, //7
    CLGD5446, //8
    CLGD5480, //9
    CLGD7541, //10
    CLGD7542, //11
    CLGD7543, //10
    CLGD7548, //12
    CLGD7555, //13
    CLGD7556  //13
} cirrus_types;

#define KNOWN_CARDS 16

//54m30
//54m40 - triple buffer, transparent color register
/*
 1 - the oldest BitBLT capable chips, !!for color-expand with transparency
 transparent color is used !!, maxwidth 2047, maxpitch 4095, maxheight 1023,
 all BitBLT registers except src&dstaddr are preserved, allow at most 7 bits
 to be discarded with color expansion at the end of each scanline, color
 expansion and hw cursor in 8 and 15/16bpp supported
 2 - supports MMIO at b8000 or at the end of linear address space (2mb-256),
 !!for color-expand with transparency 0 bits are just skipped!!, maxpitch 8191, color expansion
 with left edge clipping&pattern vertical preset, supports color expanded
 pattern polygon fills
 3 - like 2, ? need srcaddr to be written for color expansion system to
 screen ?, hw cursor also at 24/32bpp
 4 - like 3, but 64 bit, maxwidth 8191, doesn't support clipping&vertical
 preset and polygon fills, color expansion in 32bpp supported, for color
 expansion with transparency all 4 bytes of fg color has to be written and 4
 bytes of color has to be filled with not fg color, has bug in system to
 display memory transfers, hw cursor also at 24/32bpp,
 5 - the bug is corrected
 6 - like 2, but 64 bit, maxwidth 8191, maxheight 2047,supports solid fill,
 color expansion in 24 and 32bpp, for 24bpp color expansion transparency must
 be enabled, but can invert the meaning of input data, so normal color
 expansion can de done in two passes, has auto-start capability, hw cursor
 also at 24/32bpp, triple buffer ?
 7 - like 3 with video features
 8 - like 9, no clipping, no X-Y pos.
 9 - like 6, has clip rectangle, X-Y positioning, command list with possible
 interrupt on completion, left edge clipping more complex, can probably triple
 buffer, color expand with uses transparent color, no transparent mask
 10 - (7541,7543) like 1, height is _not_ preserved
 11 - (7542) don't know anything
 12 - (7548) like 1 with MMIO, autostart, height is _not_ preserved
 13 - (7555-6) like 6, MMIO in PCI 0x14 or offset 0x3fff00, triple buffer
 GR16-17, no 32bpp color expansion, autostart
 if banked GR6[3:2]=01,SR17[6]=0,SR17[7:4]=0
 else SR17[6]=1,SR17[7:4]!=0
 */

typedef struct {
    cirrus_types model;
    int biosnum,pcinum;
    char *desc;
    int family;
} CIRRUS_DETECT;

extern unsigned long af_mmio;

#define GRX 0x3ce
#define _crtc 0x3d4

__inline__ void _vsync_out_h()
{
    do {
    } while (inb(0x3DA) & 1);
}

/* _vsync_out_v:
 *  Waits until the VGA is not in a vertical retrace.
 */
__inline__ void _vsync_out_v()
{
    do {
    } while (inb(0x3DA) & 8);
}


/* _vsync_in:
 *  Waits until the VGA is in the vertical retrace period.
 */
__inline__ void _vsync_in()
{
    do {
    } while (!(inb(0x3DA) & 8));
}


/* _write_hpp:
 *  Writes to the VGA pelpan register.
 */
__inline__ void _write_hpp(int value)
{
    write_vga_register(0x3C0, 0x33, value);
}

#define outm1(index, value) *(volatile char *)(af_mmio + index) = (value)
#define outm2(index, value) *(volatile short *)(af_mmio + index) = (value)
#define outm4(index, value) *(volatile long *)(af_mmio + index) = (value)
#define inmb(index) *(volatile char *)(af_mmio + index)

__inline__ void outp1(unsigned short port,unsigned char index,unsigned char value)
{
    unsigned short w;

    w=index;
    w|=value<<8;
    outw(port,w);
}

__inline__ void outp2(unsigned short port,unsigned char index,unsigned short value)
{
    unsigned short w;

    w=index;
    w|=((value&0xff)<<8);
    outw(port,w);
    w=index+1;
    w|=(value&0xff00);
    outw(port,w);
}

__inline__ void outp3(unsigned short port,unsigned char index,unsigned long value)
{
    unsigned short w;

    w=index;
    w|=(value&0xff)<<8;
    outw(port,w);
    w=index+1;
    w|=(value&0xff00);
    outw(port,w);
    w=index+2;
    w|=(value>>8)&0xff00;
    outw(port,w);
}

#define DISABLE() asm volatile ("cli");
#define ENABLE() asm volatile ("sti");

#define CIR_FORG8(color)    outp1(GRX,0x01,(color))

#define CIR_FORG16(color)   outp1(GRX,0x01,(color) & 0xff);                  \
    outp1(GRX,0x11,((color)>>8) & 0xff)
#define CIR_FORG24(color)   outp1(GRX,0x01,(color) & 0xff);                  \
    outp1(GRX,0x11,((color)>>8) & 0xff);             \
    outp1(GRX,0x13,((color)>>16) & 0xff)
#define CIR_FORG32(color)   outp1(GRX,0x01,(color) & 0xff);                  \
    outp1(GRX,0x11,((color)>>8) & 0xff);             \
    outp1(GRX,0x13,((color)>>16) & 0xff);            \
    outp1(GRX,0x15,((color)>>24) & 0xff)

#define CIR_BACKG8(color)   outp1(GRX,0x00,(color))
#define CIR_BACKG16(color)  outp1(GRX,0x00,(color) & 0xff);                  \
    outp1(GRX,0x10,((color)>>8) & 0xff)
#define CIR_BACKG24(color)  outp1(GRX,0x00,(color) & 0xff);                  \
    outp1(GRX,0x10,((color)>>8) & 0xff);             \
    outp1(GRX,0x12,((color)>>16) & 0xff)
#define CIR_BACKG32(color)  outp1(GRX,0x00,(color) & 0xff);                  \
    outp1(GRX,0x10,((color)>>8) & 0xff);             \
    outp1(GRX,0x12,((color)>>16) & 0xff);            \
    outp1(GRX,0x14,((color)>>24) & 0xff)

#define CIR_FORG8MMIO(color) outm1(0x04,(color))
#define CIR_FORG16MMIO(color) outm2(0x04,(color))
#define CIR_FORG24MMIO(color) outm4(0x04,(color))
#define CIR_FORG32MMIO(color) outm4(0x04,(color))

#define CIR_BACKG8MMIO(color) outm1(0x00,(color))
#define CIR_BACKG16MMIO(color) outm2(0x00,(color))
#define CIR_BACKG24MMIO(color) outm4(0x00,(color))
#define CIR_BACKG32MMIO(color) outm4(0x00,(color))

#define SET_WIDTH_HEIGHTMMIO(width,height) outm4(0x08,(width)|(((height)<<16)))

#define SET_PITCHESMMIO(src,dst) outm4(0x0c,(dst)|((src)<<16)

#define SET_DSTADDRMMIO(address) outm4(0x10,address)

/* in last byte of is left edge clipping - to be added */
#define SET_SRCADDRMMIO(address) outm4(0x14,address)

#define CIR_ROP_MODEMMIO(rop,mode) outm4(0x18,(mode)|((rop)<<16))

#define CIR_BLTROPMMIO(rop) outm1(0x1a,rop)
#define CIR_BLTMODEMMIO(mode) outm1(0x18,mode)

#define CIR_WIDTH(width)    outp2(GRX,0x20,width)
#define CIR_HEIGHT(height)  outp2(GRX,0x22,height)
#define CIR_DSTPITCH(pitch) outp2(GRX,0x24,pitch)
#define CIR_SRCPITCH(pitch) outp2(GRX,0x26,pitch)
#define SET_DSTADDR(address) outp3(GRX,0x28,address)
#define SET_SRCADDR(address) outp3(GRX,0x2c,address)
#define CIR_BLTMODE(mode)   outp1(GRX,0x30,mode)
#define CIR_BLTROP(rop)     outp1(GRX,0x32,rop)
#define CIR_TRANS(color)    outp2(GRX,0x34,(color));                         \
    outp2(GRX,0x38,0);
#define CIR_TRANSMMIO(color) outm2(0x34,(color))

#define CIR_CMD(cmd)        outp1(GRX,0x31,cmd)


#define CIR_CMDMMIO(cmd)    outm1(0x40,cmd)

#define CIR_BLT_PIX8 0
#define CIR_BLT_PIX15 16
#define CIR_BLT_PIX16 16
#define CIR_BLT_PIX24 32
#define CIR_BLT_PIX32 48

#define CIR_ROP_COPY 0x0d
#define CIR_ROP_XOR 0x59
#define CIR_ROP_AND 0x05
#define CIR_ROP_OR 0x6d
#define CIR_ROP_NOP 0x00

#define CIR_CMD_RUN 0x02
#define CIR_CMD_BUSY 0x01
#define CIR_BLT_BACK 0x01
#define CIR_BLT_TRANS 0x08
#define CIR_BLT_PATT 0x40
#define CIR_BLT_MEM 0x04
#define CIR_BLT_COLEXP 0x80

#if 0
#define my_int(num,regs) asm("\
    pushal;              \
    pushl %%esi;         \
    movl (%%esi),%%edi;  \
    movl 8(%%esi),%%ebp; \
    movl 16(%%esi),%%ebx;\
    movl 20(%%esi),%%edx;\
    movl 24(%%esi),%%ecx;\
    movl 28(%%esi),%%eax;\
    pushl 4(%%esi);      \
    popl %%esi;          \
    int %1;              \
    xchgl %%esi,(%%esp); \
    movl %%eax,28(%%esi);\
    movl %%ecx,24(%%esi);\
    movl %%edx,20(%%esi);\
    movl %%ebx,16(%%esi);\
    movl %%ebp,8(%%esi); \
    movl %%edi,(%%esi);  \
    popl %%eax;          \
    movl %%eax,4(%%esi); \
    popal"               \
    ::"S" (regs), "i" (num))
#endif

#endif // CIR_DEFS_H



