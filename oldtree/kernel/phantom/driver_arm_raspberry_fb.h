


struct Bcm2835FrameBuffer
{
    // Framebuffer size
    u_int32_t	width; 
    u_int32_t    height; 

    // Framebuffer virt size
    u_int32_t    vwidth; 
    u_int32_t    vheight; 

    // GPU fills this in; set to zero
    u_int32_t    pitch;

    // Bits per pixel; set to 24
    u_int32_t    depth; 

    // Offset (must be zero?)
    u_int32_t    x;
    u_int32_t    y;

    // GPU fills this in to be a pointer to the frame buffer
    u_int32_t    pointer; 

    u_int32_t    size; 
};


#define MAIL_BASE 0xB880 /* This is the base address for the mailbox registers
	(actually, there's more than one mailbox, but this is the one we care about) */

/* Registers from mailbox 0 that we use */
#define MAIL_READ 0x00 /* We read from this register */
#define MAIL_WRITE 0x20 /* This is where we write to; it is actually the read/write of the other mailbox */
#define MAIL_STATUS 0x18 /* Status register for this mailbox */
//#define MAIL_CONFIG 0x1C - we don't actually use this, but it exists

//This bit is set in the status register if there is no space to write into the mailbox
#define MAIL_FULL 0x80000000
//This bit is set if there is nothing to read from the mailbox
#define MAIL_EMPTY 0x40000000

#define MAIL_FB 1 /* The frame buffer uses channel 1 */


