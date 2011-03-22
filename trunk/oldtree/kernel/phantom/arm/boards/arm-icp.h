
#define ICP_IDFIELD                             0xCB000000


#define ICP_PRI_INTERRUPT_IRQ_STATUS            0x14000000
#define ICP_PRI_INTERRUPT_IRQ_RAW_STATUS        0x14000004

#define ICP_PRI_INTERRUPT_IRQ_SET               0x14000008
#define ICP_PRI_INTERRUPT_IRQ_CLEAR             0x1400000C

#define ICP_PRI_INTERRUPT_SOFT_SET              0x14000010
#define ICP_PRI_INTERRUPT_SOFT_CLEAR            0x14000014


#define ICP_PRI_INTERRUPT_FIQ_STATUS            0x14000020
#define ICP_PRI_INTERRUPT_FIQ_RAW_STATUS        0x14000024

#define ICP_PRI_INTERRUPT_FIQ_SET               0x14000028
#define ICP_PRI_INTERRUPT_FIQ_CLEAR             0x1400002C



#define SIC_INT_STATUS                          0xCA000000
#define SIC_INT_RAWSTAT                         0xCA000004
#define SIC_INT_ENABLESET                       0xCA000008
#define SIC_INT_ENABLECLR                       0xCA00000C
#define SIC_INT_SOFTSET                         0xCA000010
#define SIC_INT_SOFTCLR                         0xCA000014



