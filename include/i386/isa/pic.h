#ifndef PIC_H
#define PIC_H

void	phantom_pic_init(unsigned char master_base, unsigned char slave_base);
void 	phantom_interrupt_ack(int n);
void	phantom_pic_disable_irq(u_int8_t irq_no);
void	phantom_pic_enable_irq(u_int8_t irq_no);
int 	phantom_pic_is_irq_pending(u_int8_t irq_no);






#endif // PIC_H

