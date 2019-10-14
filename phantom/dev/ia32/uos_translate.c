/*
 * Driver for PS/2 keyboard.
 *
 * Copyright (C) 2005 Serge Vakulenko
 *
 * Specifications of PS/2 keyboard interface are available
 * at http://www.computer-engineering.org/ps2keyboard/
 */

#include <compat/uos.h>
#include <compat/uos/keyboard.h>


static unsigned fix_qemu_arrows_bug(unsigned k)
{
	switch(k)
	{
		default: return k;
		case KEY_KP0: return KEY_INSERT;
		case KEY_KP1: return KEY_END;
		case KEY_KP2: return KEY_DOWN;
		case KEY_KP3: return KEY_PAGEDOWN;
		case KEY_KP4: return KEY_LEFT;
		//case KEY_KP5: return KEY_;
		case KEY_KP6: return KEY_RIGHT;
		case KEY_KP7: return KEY_HOME;
		case KEY_KP8: return KEY_UP;
		case KEY_KP9: return KEY_PAGEUP;

		case KEY_KP_PERIOD: return KEY_DELETE;
	}
}

/*
 * Process a keyboard event.
 * Process control chars, shift and caps lock, and num lock modifiers.
 * Do not process alt and meta modifiers.
 */
void
keyboard_translate (keyboard_event_t *m)
{
	switch (m->key) {
	case '\b':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = 0x7f;		/* Backspace -> Del */
		break;

	//case '\r':
	case '\n':
		if (m->modifiers & KEYMOD_CTRL)
			//m->key = '\n';		/* Enter -> Newline */
			m->key = '\r';		/* much better to me [dz] */
		break;

        case '1':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '!';		/* Shift 1 -> ! */
		break;

        case '2':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = '@' & 0x1f;	/* Ctrl 2 -> ^@ */
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '@';		/* Shift 2 -> @ */
		break;

        case '3':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '#';		/* Shift 3 -> # */
		break;

        case '4':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '$';		/* Shift 4 -> $ */
		break;

        case '5':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '%';		/* Shift 5 -> % */
		break;

        case '6':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = '^' & 0x1f;	/* Ctrl 6 -> ^^ */
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '^';		/* Shift 6 -> ^ */
		break;

        case '7':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '&';		/* Shift 7 -> & */
		break;

        case '8':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '*';		/* Shift 8 -> * */
		break;

        case '9':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '(';		/* Shift 9 -> ( */
		break;

        case '0':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = ')';		/* Shift 0 -> ) */
		break;

        case '-':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = '_' & 0x1f;	/* Ctrl - -> ^_ */
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '_';		/* Shift - -> _ */
		break;

        case '=':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '+';		/* Shift = -> + */
		break;

        case '[':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = '[' & 0x1f;	/* Ctrl [ -> ^[ */
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '{';		/* Shift [ -> { */
		break;

        case ']':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = ']' & 0x1f;	/* Ctrl ] -> ^] */
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '}';		/* Shift ] -> } */
		break;

        case ';':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = ':';		/* Shift ; -> : */
		break;

        case '\'':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '"';		/* Shift ' -> " */
		break;

        case '`':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '~';		/* Shift ` -> ~ */
		break;

        case '\\':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = '\\' & 0x1f;	/* Ctrl \ -> ^\ */
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '|';		/* Shift \ -> | */
		break;

        case ',':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '<';		/* Shift , -> < */
		break;

        case '.':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '>';		/* Shift . -> > */
		break;

        case '/':
		if (m->modifiers & KEYMOD_CTRL)
			m->key = KEY_UNKNOWN;
		else if (m->modifiers & KEYMOD_SHIFT)
			m->key = '?';		/* Shift / -> ? */
		break;

	case 'A': case 'B': case 'C': case 'D':	case 'E':
	case 'F': case 'G': case 'H': case 'I': case 'J':
	case 'K': case 'L': case 'M': case 'N': case 'O':
	case 'P': case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X': case 'Y':
	case 'Z':
		if (m->modifiers & KEYMOD_CTRL)
			m->key &= 0x1f;
		else if (! (m->modifiers & (KEYMOD_SHIFT | KEYMOD_CAPS)))
			m->key += 'a' - 'A';
		break;

    case KEY_KP0: case KEY_KP1: case KEY_KP2: case KEY_KP3:
	case KEY_KP4: case KEY_KP5: case KEY_KP6: case KEY_KP7:
	case KEY_KP8: case KEY_KP9:
		if (m->modifiers & KEYMOD_NUM)
			m->key += '0' - KEY_KP0; /* Num KP N -> N */
		else
			m->key = fix_qemu_arrows_bug(m->key);
		break;

        case KEY_KP_PERIOD:
		if (m->modifiers & KEYMOD_NUM)
			m->key = '.';		/* Num KP . -> . */
		else
			m->key = fix_qemu_arrows_bug(m->key);
		break;

        case KEY_KP_DIVIDE:
		if (m->modifiers & KEYMOD_NUM)
			m->key = '/';		/* Num KP / -> / */
		break;

        case KEY_KP_MULTIPLY:
		if (m->modifiers & KEYMOD_NUM)
			m->key = '*';		/* Num KP * -> * */
		break;

        case KEY_KP_MINUS:
		if (m->modifiers & KEYMOD_NUM)
			m->key = '-';		/* Num KP - -> - */
		break;

        case KEY_KP_PLUS:
		if (m->modifiers & KEYMOD_NUM)
			m->key = '+';		/* Num KP + -> + */
		break;

        case KEY_KP_EQUALS:
		if (m->modifiers & KEYMOD_NUM)
			m->key = '=';		/* Num KP = -> = */
		break;

	case KEY_KP_ENTER:
		if (m->modifiers & KEYMOD_CTRL)
			m->key = '\n';		/* KP Enter -> Newline */
		else
			m->key = '\r';		/* KP Enter -> Return */
		break;

	case KEY_NUMLOCK:
	case KEY_CAPSLOCK:
	case KEY_LSHIFT:
	case KEY_RSHIFT:
	case KEY_LCTRL:
	case KEY_RCTRL:
	case KEY_LALT:
	case KEY_RALT:
	case KEY_LMETA:
	case KEY_RMETA:
		/* Filter out modifiers. */
		m->key = KEY_UNKNOWN;
		break;
	}
}
