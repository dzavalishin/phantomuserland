#ifndef PIC_H
#define PIC_H

void	phantom_pic_init(unsigned char master_base, unsigned char slave_base);
void 	phantom_interrupt_ack(unsigned int n);

void	phantom_pic_disable_irq(unsigned int irq_no);
void	phantom_pic_enable_irq(unsigned int irq_no);

int     phantom_pic_get_irqmask(void);
void	phantom_pic_set_irqmask(int mask);

void	phantom_pic_disable_all(void);

int 	phantom_pic_is_irq_pending(unsigned int irq_no);

#endif // PIC_H

