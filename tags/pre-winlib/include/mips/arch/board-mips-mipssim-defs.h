// 
// TODO in fact, 0xB4000000 must be added in bus output functions
//#define BOARD_ISA_IO 0xBfd00000
//#define BOARD_ISA_IO_LEN 0x10000


/* MIPSnet register offsets */

#define MIPSNET_DEV_ID		0x00

#  define MIPSNET_DEV_ID_VALUE_LO 0x4d495053
#  define MIPSNET_DEV_ID_VALUE_HI 0x4e455430

#define MIPSNET_BUSY		0x08
#define MIPSNET_RX_DATA_COUNT	0x0c
#define MIPSNET_TX_DATA_COUNT	0x10

#define MIPSNET_INT_CTL		0x14
# define MIPSNET_INTCTL_TXDONE		0x00000001
# define MIPSNET_INTCTL_RXDONE		0x00000002
# define MIPSNET_INTCTL_TESTBIT		0x80000000

// Unused
#define MIPSNET_INTERRUPT_INFO	0x18 

#define MIPSNET_RX_DATA_BUFFER	0x1c
#define MIPSNET_TX_DATA_BUFFER	0x20
