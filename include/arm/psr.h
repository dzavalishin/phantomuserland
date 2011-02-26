/* Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs */
#define USR_MODE 0x10 /* User Mode */
#define FIQ_MODE 0x11 /* FIQ Mode */
#define IRQ_MODE 0x12 /* IRQ Mode */
#define SVC_MODE 0x13 /* Supervisor Mode */
#define ABT_MODE 0x17 /* Abort Mode */
#define UND_MODE 0x1B /* Undefined Mode */
#define SYS_MODE 0x1F /* System Mode */

#define I_BIT    0x80 /* when I bit is set, IRQ is disabled */
#define F_BIT    0x40 /* when F bit is set, FIQ is disabled */
