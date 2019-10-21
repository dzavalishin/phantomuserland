
#define KBD_INBUFSZ	20

typedef struct _keyboard_ps2_t {
	//keyboard_interface_t *interface;
        //mutex_t lock;

        hal_spinlock_t lock;

	//ARRAY (stack, KBD_STACKSZ);		/* task receive stack */
	keyboard_event_t in_buf [KBD_INBUFSZ];	/* keyboard event queue */
	keyboard_event_t *in_first, *in_last;	/* queue pointers */
	int rate;				/* chars per second */
	int delay;				/* milliseconds */
	int leds;				/* num, caps, scroll locks */
	int state;				/* state of scan decoder */
	//int state_E012; // print screen prefix
	//int state_E012_7C; // print screen prefix
	int state_E114; // pause prefix
	int state_F0; // break modifier
	int modifiers;				/* state of modifiers */
	//int ctrl_alt_del;			/* reboot flag */
	int capslock;				/* caps lock pressed */
	int numlock;				/* num lock pressed */
	int national_keymap;        // National (such as cyrillic) key map selected
} keyboard_ps2_t;

//void keyboard_ps2_init (keyboard_ps2_t *u, int prio);

