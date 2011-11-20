#ifndef __PL050_KMI_H__
#define __PL050_KMI_H__
 
/* Register offsets */
#define PL050_KMICR		0x00
#define PL050_KMISTAT		0x04
#define PL050_KMIDATA		0x08
#define PL050_KMICLKDIV		0x0C
#define PL050_KMIIR		0x10
 
/* Bit definitions for KMI control register */
#define KMI_TYPE		(1 << 0x5)
#define	KMI_RXINTR		(1 << 0x4)
#define KMI_TXINTR          	(1 << 0x3)
#define KMI_EN			(1 << 0x2)
#define KMI_FD			(1 << 0x1)
#define	KMI_FC			(1 << 0x0)

#define PL050_KMIIR_RX_INTR     (1<<0)
#define PL050_KMIIR_TX_INTR     (1<<1)

/* KMI generic defines */
#define KMI_DATA_RESET     0xFF
#define KMI_DATA_RTR       0xAA
 
/* Keyboard special defines */
#define KYBD_DATA_RESET     KMI_DATA_RESET	// Keyboard reset
#define KYBD_DATA_RTR       KMI_DATA_RTR	// Keyboard response to reset
 
#define KYBD_DATA_KEYUP		0xF0	// Key up control code
#define KYBD_DATA_SHIFTL	18    	// Shift key left
#define KYBD_DATA_SHIFTR	89    	// Shift key right
 
/* Bit definitions for KMI STAT register */
#define KMI_TXEMPTY	(1 << 0x6)
#define KMI_TXBUSY	(1 << 0x5)
#define KMI_RXFULL	(1 << 0x4)
#define KMI_RXBUSY	(1 << 0x3)
#define KMI_RXPARITY	(1 << 0x2)
#define KMI_CLKIN	(1 << 0x1)
#define KMI_DATAIN	(1 << 0x0)
 
/* Mouse special defines */
#define MOUSE_DATA_RESET	KMI_DATA_RESET	// Mouse reset
#define MOUSE_DATA_RTR		KMI_DATA_RTR	// Mouse response to reset
#define MOUSE_DATA_ACK		0xFA
#define MOUSE_DATA_ENABLE   	0xF4    	// Mouse enable

#if 0
/* Common functions */
void kmi_rx_irq_enable(unsigned long base);
int kmi_data_read(unsigned long base);
 
/* Keyboard specific calls */
char kmi_keyboard_read(unsigned long base, struct keyboard_state *state);
void kmi_keyboard_init(unsigned long base, unsigned int div);
 
/* Mouse specific calls */
void kmi_mouse_enable(unsigned long base);
void kmi_mouse_init(unsigned long base, unsigned int div);
#endif


#endif /* __PL050_KMI_H__ */
