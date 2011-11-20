all: zb_asm.o

zb.S: zb.c
	gcc -S zb.c
        
zb_asm.o: zb_asm.S
	gcc -c zb_asm.S
