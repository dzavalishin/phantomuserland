#ifndef _UOS_KEYB_H
#define _UOS_KEYB_H

/*
 * Generic keyboard interface.
 */
typedef struct _keyboard_event_t {
	unsigned short key;		/* Unicode symbol */
	unsigned short modifiers;	/* ctrl, alt etc. */
	unsigned short release;		/* 0 - press, 1 - release */
} keyboard_event_t;


#define KEY_IS_FUNC(key) (((key) & 0xFF00) == 0xF800 )

/*
 * Keysyms 0..126 are mapped to ASCII
 */
#define KEY_BACKSPACE   8
#define KEY_TAB         9
#define KEY_ENTER       '\n'
#define KEY_ESCAPE      27

#define KEY_UNKNOWN           	0xFFFF

/*
 * Following keysyms are mapped to private use portion of Unicode-16
 */
#define KEY_FIRST		0xF800	/* arrows + home/end pad */
#define KEY_LEFT		0xF800
#define KEY_RIGHT		0xF801
#define KEY_UP			0xF802
#define KEY_DOWN		0xF803
#define KEY_INSERT		0xF804
#define KEY_DELETE		0xF805
#define KEY_HOME		0xF806
#define KEY_END			0xF807
#define KEY_PAGEUP		0xF808
#define KEY_PAGEDOWN		0xF809

#define KEY_KP0			0xF80A	/* numeric keypad */
#define KEY_KP1			0xF80B
#define KEY_KP2			0xF80C
#define KEY_KP3			0xF80D
#define KEY_KP4			0xF80E
#define KEY_KP5			0xF80F
#define KEY_KP6			0xF810
#define KEY_KP7			0xF811
#define KEY_KP8			0xF812
#define KEY_KP9			0xF813
#define KEY_KP_PERIOD		0xF814
#define KEY_KP_DIVIDE		0xF815
#define KEY_KP_MULTIPLY		0xF816
#define KEY_KP_MINUS		0xF817
#define KEY_KP_PLUS		0xF818
#define KEY_KP_ENTER		0xF819
#define KEY_KP_EQUALS		0xF81A

#define KEY_F1			0xF81B	/* function keys */
#define KEY_F2			0xF81C
#define KEY_F3			0xF81D
#define KEY_F4			0xF81E
#define KEY_F5			0xF81F
#define KEY_F6			0xF820
#define KEY_F7			0xF821
#define KEY_F8			0xF822
#define KEY_F9			0xF823
#define KEY_F10			0xF824
#define KEY_F11			0xF825
#define KEY_F12			0xF827

#define KEY_NUMLOCK		0xF828	/* key state modifier keys*/
#define KEY_CAPSLOCK		0xF829
#define KEY_SCROLLOCK		0xF82A
#define KEY_LSHIFT		0xF82B
#define KEY_RSHIFT		0xF82C
#define KEY_LCTRL		0xF82D
#define KEY_RCTRL		0xF82E
#define KEY_LALT		0xF82F
#define KEY_RALT		0xF830
#define KEY_LMETA		0xF831
#define KEY_RMETA		0xF832
#define KEY_ALTGR		0xF833

#define KEY_PRINT		0xF834	/* misc function keys */
#define KEY_SYSREQ		0xF835
#define KEY_PAUSE		0xF836
#define KEY_BREAK		0xF837
#define KEY_QUIT		0xF838	/* virtual key */
#define KEY_MENU		0xF839	/* virtual key */
#define KEY_REDRAW		0xF83A	/* virtual key */

#define KEY_CONTRAST		0xF842	/* handheld function keys */
#define KEY_BRIGHTNESS		0xF843
#define KEY_SELECTUP		0xF844
#define KEY_SELECTDOWN		0xF845
#define KEY_ACCEPT		0xF846
#define KEY_CANCEL		0xF847
#define KEY_APP1		0xF848
#define KEY_APP2		0xF849
#define KEY_APP3		0xF84A
#define KEY_APP4		0xF84B
#define KEY_SUSPEND		0xF84C
#define KEY_END_NORMAL		0xF84D	/* insert additional keys before this */

/*
 * The following keys are useful for remote controls on consumer
 * electronics devices (e.g. TVs, videos, DVD players, cable
 * boxes, satellite boxes, digital terrestrial recievers, ...)
 *
 * The codes are taken from the HAVi specification:
 * HAVi Level 2 User Interface version 1.1, May 15th 2001
 * They are listed in section 8.7.
 *
 * For more information see http://www.havi.org/
 */

/* Key code for first HAVi key */
#define KEY_HAVI_BASE	(KEY_END_NORMAL+1)

/* HAVi code for first HAVi key */
#define KEY_HAVI_FIRST	403

/* HAVi code for last HAVi key */
#define KEY_HAVI_LAST	460

/* HRcEvent.VK_... code to KEY_... code */
#define KEY_FROM_HAVI(h)	((h) + (KEY_HAVI_BASE - KEY_HAVI_FIRST))

/* KEY_... code to HRcEvent.VK_... code */
#define KEY_TO_HAVI(m)	((m) - (KEY_HAVI_BASE - KEY_HAVI_FIRST))

/* Can an KEY_... code be converted into a HRcEvent.VK_... code? */
#define KEY_IS_HAVI(m)	((unsigned)((m) - KEY_HAVI_BASE) \
	<= (unsigned)(KEY_HAVI_LAST - KEY_HAVI_FIRST))

#define KEY_COLORED_KEY_0         KEY_FROM_HAVI(403)
#define KEY_COLORED_KEY_1         KEY_FROM_HAVI(404)
#define KEY_COLORED_KEY_2         KEY_FROM_HAVI(405)
#define KEY_COLORED_KEY_3         KEY_FROM_HAVI(406)
#define KEY_COLORED_KEY_4         KEY_FROM_HAVI(407)
#define KEY_COLORED_KEY_5         KEY_FROM_HAVI(408)
#define KEY_POWER                 KEY_FROM_HAVI(409)
#define KEY_DIMMER                KEY_FROM_HAVI(410)
#define KEY_WINK                  KEY_FROM_HAVI(411)
#define KEY_REWIND                KEY_FROM_HAVI(412)
#define KEY_STOP                  KEY_FROM_HAVI(413)
#define KEY_EJECT_TOGGLE          KEY_FROM_HAVI(414)
#define KEY_PLAY                  KEY_FROM_HAVI(415)
#define KEY_RECORD                KEY_FROM_HAVI(416)
#define KEY_FAST_FWD              KEY_FROM_HAVI(417)
#define KEY_PLAY_SPEED_UP         KEY_FROM_HAVI(418)
#define KEY_PLAY_SPEED_DOWN       KEY_FROM_HAVI(419)
#define KEY_PLAY_SPEED_RESET      KEY_FROM_HAVI(420)
#define KEY_RECORD_SPEED_NEXT     KEY_FROM_HAVI(421)
#define KEY_GO_TO_START           KEY_FROM_HAVI(422)
#define KEY_GO_TO_END             KEY_FROM_HAVI(423)
#define KEY_TRACK_PREV            KEY_FROM_HAVI(424)
#define KEY_TRACK_NEXT            KEY_FROM_HAVI(425)
#define KEY_RANDOM_TOGGLE         KEY_FROM_HAVI(426)
#define KEY_CHANNEL_UP            KEY_FROM_HAVI(427)
#define KEY_CHANNEL_DOWN          KEY_FROM_HAVI(428)
#define KEY_STORE_FAVORITE_0      KEY_FROM_HAVI(429)
#define KEY_STORE_FAVORITE_1      KEY_FROM_HAVI(430)
#define KEY_STORE_FAVORITE_2      KEY_FROM_HAVI(431)
#define KEY_STORE_FAVORITE_3      KEY_FROM_HAVI(432)
#define KEY_RECALL_FAVORITE_0     KEY_FROM_HAVI(433)
#define KEY_RECALL_FAVORITE_1     KEY_FROM_HAVI(434)
#define KEY_RECALL_FAVORITE_2     KEY_FROM_HAVI(435)
#define KEY_RECALL_FAVORITE_3     KEY_FROM_HAVI(436)
#define KEY_CLEAR_FAVORITE_0      KEY_FROM_HAVI(437)
#define KEY_CLEAR_FAVORITE_1      KEY_FROM_HAVI(438)
#define KEY_CLEAR_FAVORITE_2      KEY_FROM_HAVI(439)
#define KEY_CLEAR_FAVORITE_3      KEY_FROM_HAVI(440)
#define KEY_SCAN_CHANNELS_TOGGLE  KEY_FROM_HAVI(441)
#define KEY_PINP_TOGGLE           KEY_FROM_HAVI(442)
#define KEY_SPLIT_SCREEN_TOGGLE   KEY_FROM_HAVI(443)
#define KEY_DISPLAY_SWAP          KEY_FROM_HAVI(444)
#define KEY_SCREEN_MODE_NEXT      KEY_FROM_HAVI(445)
#define KEY_VIDEO_MODE_NEXT       KEY_FROM_HAVI(446)
#define KEY_VOLUME_UP             KEY_FROM_HAVI(447)
#define KEY_VOLUME_DOWN           KEY_FROM_HAVI(448)
#define KEY_MUTE                  KEY_FROM_HAVI(449)
#define KEY_SURROUND_MODE_NEXT    KEY_FROM_HAVI(450)
#define KEY_BALANCE_RIGHT         KEY_FROM_HAVI(451)
#define KEY_BALANCE_LEFT          KEY_FROM_HAVI(452)
#define KEY_FADER_FRONT           KEY_FROM_HAVI(453)
#define KEY_FADER_REAR            KEY_FROM_HAVI(454)
#define KEY_BASS_BOOST_UP         KEY_FROM_HAVI(455)
#define KEY_BASS_BOOST_DOWN       KEY_FROM_HAVI(456)
#define KEY_INFO                  KEY_FROM_HAVI(457)
#define KEY_GUIDE                 KEY_FROM_HAVI(458)
#define KEY_TELETEXT              KEY_FROM_HAVI(459)
#define KEY_SUBTITLE              KEY_FROM_HAVI(460)

#define KEY_LAST                  KEY_SUBTITLE

/*
 * Keyboard state modifiers
 */
#define KEYMOD_LSHIFT		0x0001
#define KEYMOD_RSHIFT		0x0002
#define KEYMOD_LCTRL 		0x0040
#define KEYMOD_RCTRL 		0x0080
#define KEYMOD_LALT  		0x0100
#define KEYMOD_RALT  		0x0200
#define KEYMOD_LMETA 		0x0400		/* windows/apple key */
#define KEYMOD_RMETA 		0x0800		/* windows/apple key */
#define KEYMOD_NUM   		0x1000
#define KEYMOD_CAPS  		0x2000
#define KEYMOD_ALTGR 		0x4000
#define KEYMOD_SCR		0x8000

#define KEYMOD_CTRL		(KEYMOD_LCTRL | KEYMOD_RCTRL)
#define KEYMOD_SHIFT		(KEYMOD_LSHIFT | KEYMOD_RSHIFT)
#define KEYMOD_ALT		(KEYMOD_LALT | KEYMOD_RALT)
#define KEYMOD_META		(KEYMOD_LMETA | KEYMOD_RMETA)

/*
 * Keyboard LED values
 */
#define KEYLED_CAPS		01
#define KEYLED_NUM		02
#define KEYLED_SCROLL		04

/*
typedef struct _keyboard_t {
	struct _keyboard_interface_t *interface;

	mutex_t lock;

} keyboard_t;
*/
#if 0
typedef struct _keyboard_interface_t {
	void (*wait_event) (keyboard_t *u, keyboard_event_t *data);
	int (*get_event) (keyboard_t *u, keyboard_event_t *data);
	int (*get_modifiers) (keyboard_t *u);
	int (*get_leds) (keyboard_t *u);
	void (*set_leds) (keyboard_t *u, int leds);
	int (*get_rate) (keyboard_t *u);
	void (*set_rate) (keyboard_t *u, int cps);
	int (*get_delay) (keyboard_t *u);
	void (*set_delay) (keyboard_t *u, int msec);
} keyboard_interface_t;

/*
 * Методы приходится делать в виде макросов,
 * т.к. необходимо приведение типа к родительскому.
 * /
#define to_kbd(x) ((keyboard_t*)&(x)->interface)

#define keyboard_wait_event(x,s) (x)->interface->wait_event(to_kbd (x), s)
#define keyboard_get_event(x,s)	(x)->interface->get_event(to_kbd (x), s)
#define keyboard_get_modifiers(x) (x)->interface->get_modifiers(to_kbd (x))
#define keyboard_get_leds(x)	(x)->interface->get_leds(to_kbd (x))
#define keyboard_set_leds(x,s)	(x)->interface->set_leds(to_kbd (x), s)
#define keyboard_get_rate(x)	(x)->interface->get_rate(to_kbd (x))
#define keyboard_set_rate(x,s)	(x)->interface->set_rate(to_kbd (x), s)
#define keyboard_get_delay(x)	(x)->interface->get_delay(to_kbd (x))
#define keyboard_set_delay(x,s)	(x)->interface->set_delay(to_kbd (x), s)
*/
#endif
/*
 * Translate input key, processing control chars,
 * shift and caps lock, and num lock modifiers.
 * Do not process alt and meta modifiers.
 */
void keyboard_translate (keyboard_event_t *m);

#endif // _UOS_KEYB_H

