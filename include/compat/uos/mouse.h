/*
 * Generic mouse interface.
 */
typedef struct _mouse_move_t {
	unsigned short buttons;			/* current button state */
	signed short int dx, dy, dz;		/* change in position */
} mouse_move_t;

/* Mouse button bits */
#define MOUSE_BTN_LEFT		01
#define MOUSE_BTN_RIGHT		02
#define MOUSE_BTN_MIDDLE	04

typedef struct _mouse_t {
	struct _mouse_interface_t *interface;
#ifdef __KERNEL_UOS_H_
	mutex_t lock;
#endif
} mouse_t;

/*
typedef struct _mouse_interface_t {
	void (*wait_move) (mouse_t *u, mouse_move_t *data);
	int (*get_move) (mouse_t *u, mouse_move_t *data);
} mouse_interface_t;

 / *
 * Методы приходится делать в виде макросов,
 * т.к. необходимо приведение типа к родительскому.
 * /
#define to_mouse(x) ((mouse_t*)&(x)->interface)

#define mouse_wait_move(x,s)	(x)->interface->wait_move(to_mouse (x), s)
#define mouse_get_move(x,s)	(x)->interface->get_move(to_mouse (x), s)
*/