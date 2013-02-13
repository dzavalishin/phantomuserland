
#ifndef ASSEMBLER

void arm_raspberry_timer0_init(int freq);

void arm_bcm2835_init_interrupts(void);
void arm_bcm2835_interrupts_disable_all(void);
void arm_bcm2835_interrupt_enable(int irq);
void arm_bcm2835_interrupt_disable(int irq);

void arm_raspberry_video_init(void);


#endif
