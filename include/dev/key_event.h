#if 0
/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KEY_EVENT_H
#define _NEWOS_KEY_EVENT_H

#include <phantom_types.h>

/* standard PC modifiers */
#define KEY_MODIFIER_DOWN		0x1
#define KEY_MODIFIER_UP			0x2

/* extended keys (not alphanumeric) */
typedef enum {
	/* these are seen on PC keyboards */
	KEY_NONE = 0x80,
	KEY_RETURN,
	KEY_ESC,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LCTRL,
	KEY_RCTRL,
	KEY_LALT,
	KEY_RALT,
	KEY_CAPSLOCK,
	KEY_LWIN,
	KEY_RWIN,
	KEY_MENU,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_F13,
	KEY_F14,
	KEY_F15,
	KEY_F16,
	KEY_F17,
	KEY_F18,
	KEY_F19,
	KEY_F20,
	KEY_PRTSCRN,
	KEY_SCRLOCK,
	KEY_PAUSE,
	KEY_TAB,
	KEY_BACKSPACE,
	KEY_INS,
	KEY_DEL,
	KEY_HOME,
	KEY_END,
	KEY_PGUP,
	KEY_PGDN,
	KEY_ARROW_UP,
	KEY_ARROW_DOWN,
	KEY_ARROW_LEFT,
	KEY_ARROW_RIGHT,
	KEY_PAD_NUMLOCK,
	KEY_PAD_DIVIDE,
	KEY_PAD_MULTIPLY,
	KEY_PAD_MINUS,
	KEY_PAD_PLUS,
	KEY_PAD_ENTER,
	KEY_PAD_PERIOD,
	KEY_PAD_0,
	KEY_PAD_1,
	KEY_PAD_2,
	KEY_PAD_3,
	KEY_PAD_4,
	KEY_PAD_5,
	KEY_PAD_6,
	KEY_PAD_7,
	KEY_PAD_8,
	KEY_PAD_9,

	/* seen on some non-pc keyboards */
	KEY_S1,
	KEY_S2,
	KEY_S3,

	/* NeXT keyboard */
	KEY_LHELP,
	KEY_RHELP,

	/* SUN Type 4 */
	KEY_COMPOSE,
	KEY_ALT_GRAPH,
	KEY_HELP,
	KEY_STOP,
	KEY_PROPS,
	KEY_FRONT,
	KEY_OPEN,
	KEY_FIND,
	KEY_AGAIN,
	KEY_UNDO,
	KEY_COPY,
	KEY_PASTE,
	KEY_CUT,

	/* SUN Type 6 */
	KEY_BLANK,
} extended_keys;

#define KEY_LED_SCROLL                  0x1
#define KEY_LED_NUM                     0x2
#define KEY_LED_CAPS                    0x4
#define KEY_LED_COMPOSE                 0x8	/* on SUN Type 4 */

#define _KEYBOARD_IOCTL_SET_LEDS 10000

typedef struct _key_event {
	u_int16_t keycode;
	u_int16_t modifiers;
	u_int16_t keychar;
} _key_event;

void phantom_dev_keyboard_get_key( _key_event *out);

//! keyboard pipeline entry point - USB keyb driver pushes scancodes here
void handle_keycode(unsigned char key);


#endif

#endif

