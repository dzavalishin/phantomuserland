#define MOUSE_INBUFSZ	20

typedef struct _mouse_ps2_t {
	//mouse_interface_t *interface;

        //mutex_t lock;
        hal_spinlock_t lock;

	//ARRAY (stack, MOUSE_STACKSZ);		/* task receive stack */
	mouse_move_t in_buf [MOUSE_INBUFSZ];	/* mouse event queue */
	mouse_move_t *in_first, *in_last;	/* queue pointers */
	unsigned char buf [4];			/* raw mouse data */
	int count;				/* counter of raw data */
	int wheel;				/* intellimouse mode */
} mouse_ps2_t;

//void mouse_ps2_init (mouse_ps2_t *u, int prio);
