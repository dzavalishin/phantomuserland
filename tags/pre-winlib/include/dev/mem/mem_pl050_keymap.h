#ifndef __KEYMAP_H__
#define __KEYMAP_H__
 
/* Special meaning keys */
#define KEYCODE_LSHIFT      0x101
#define KEYCODE_RSHIFT      0x102
#define KEYCODE_LCTRL       0x103
#define KEYCODE_RCTRL       0x104
#define KEYCODE_ALT         0x105
#define KEYCODE_ALTGR       0x106
 
#define KEYCODE_CAPSLK      0x201
#define KEYCODE_SCRLK       0x202
#define KEYCODE_NUMLK       0x203
 
#define KEYCODE_TAB         0x301
#define KEYCODE_BACKSP      0x302
#define KEYCODE_RETURN      0x303
#define KEYCODE_ESCAPE      0x304
#define KEYCODE_ENTER       0x305
 
#define KEYCODE_PRTSCR      0x401
#define KEYCODE_BREAK       0x402
#define KEYCODE_INSERT      0x403
#define KEYCODE_HOME        0x404
#define KEYCODE_PAGEUP      0x405
#define KEYCODE_DELETE      0x406
#define KEYCODE_END         0x407
#define KEYCODE_PAGEDN      0x408
 
#define KEYCODE_UP          0x501
#define KEYCODE_DOWN        0x502
#define KEYCODE_LEFT        0x503
#define KEYCODE_RIGHT       0x504
#define KEYCODE_CENTER      0x505
 
#define KEYCODE_F1          0x601
#define KEYCODE_F2          0x602
#define KEYCODE_F3          0x603
#define KEYCODE_F4          0x604
#define KEYCODE_F5          0x605
#define KEYCODE_F6          0x606
#define KEYCODE_F7          0x607
#define KEYCODE_F8          0x608
#define KEYCODE_F9          0x609
#define KEYCODE_F10         0x60A
#define KEYCODE_F11         0x60B
#define KEYCODE_F12         0x60C
 
#define KEYCODE_WINL        0x701
#define KEYCODE_WINR        0x702
#define KEYCODE_MENU        0x703
 
#define MODIFIER_EXTENDED   0x00100000
#define MODIFIER_EXTENDED2  0x00200000
#define MODIFIER_RCTRL      0x00400000
#define MODIFIER_RSHIFT     0x00800000
#define MODIFIER_LSHIFT     0x01000000
#define MODIFIER_LCTRL      0x02000000
#define MODIFIER_ALT        0x04000000
#define MODIFIER_ALTGR      0x08000000
#define MODIFIER_SCRLK      0x10000000
#define MODIFIER_NUMLK      0x20000000
#define MODIFIER_CAPSLK     0x40000000
#define MODIFIER_RELEASE    0x80000000
#define MODIFIER_SHIFT      (MODIFIER_LSHIFT | MODIFIER_RSHIFT)
#define MODIFIER_CTRL       (MODIFIER_LCTRL | MODIFIER_RCTRL)
 
struct keyboard_key {
	int nomods;
	int shift;
	int ext_nomods;
	int ext_shift;
};
 
/*
 * Keymap for a UK keyboard
 * maps key numbers->key codes
 *
 * We will use scan code index to get the key
 *
 * FIXME: element 1 and 4 gives, muticharacter
 * character constant error, fix this.
 */
struct keyboard_key keymap_uk2[256] = {
/*  0    */     {0,0,0,0},
#if 0
/*  1    */     {'`','¬',0,0},
#else
/*  1    */     {'`',0,0,0},
#endif
/*  2    */     {'1','!',0,0},
/*  3    */     {'2','"',0,0},
#if 0
/*  4    */     {'3','?',0,0},
#else
/*  4    */     {'3',0,0,0},
#endif
/*  5    */     {'4','$',0,0},
/*  6    */     {'5','%',0,0},
/*  7    */     {'6','^',0,0},
/*  8    */     {'7','&',0,0},
/*  9    */     {'8','*',0,0},
/*  10   */     {'9','(',0,0},
/*  11   */     {'0',')',0,0},
/*  12   */     {'-','_',0,0},
/*  13   */     {'=','+',0,0},
/*  14   */     {0,0,0,0},
/*  15   */     {KEYCODE_BACKSP,0,0,0},
/*  16   */     {KEYCODE_TAB,0,0,0},
/*  17   */     {'q','Q',0,0},
/*  18   */     {'w','W',0,0},
/*  19   */     {'e','E',0,0},
/*  20   */     {'r','R',0,0},
/*  21   */     {'t','T',0,0},
/*  22   */     {'y','Y',0,0},
/*  23   */     {'u','U',0,0},
/*  24   */     {'i','I',0,0},
/*  25   */     {'o','O',0,0},
/*  26   */     {'p','P',0,0},
/*  27   */     {'[','{',0,0},
/*  28   */     {']','}',0,0},
/*  29   */     {'#','~',0,0},
/*  30   */     {KEYCODE_CAPSLK,0,0,0},
/*  31   */     {'a','A',0,0},
/*  32   */     {'s','S',0,0},
/*  33   */     {'d','D',0,0},
/*  34   */     {'f','F',0,0},
/*  35   */     {'g','G',0,0},
/*  36   */     {'h','H',0,0},
/*  37   */     {'j','J',0,0},
/*  38   */     {'k','K',0,0},
/*  39   */     {'l','L',0,0},
/*  40   */     {';',':',0,0},
/*  41   */     {'\'','@',0,0},
/*  42   */     {0,0,0,0},
/*  43   */     {'\n','\n',KEYCODE_ENTER,0},
/*  44   */     {KEYCODE_LSHIFT,0,0,0},
/*  45   */     {'\\','|',0,0},
/*  46   */     {'z','Z',0,0},
/*  47   */     {'x','X',0,0},
/*  48   */     {'c','C',0,0},
/*  49   */     {'v','V',0,0},
/*  50   */     {'b','B',0,0},
/*  51   */     {'n','N',0,0},
/*  52   */     {'m','M',0,0},
/*  53   */     {',','<',0,0},
/*  54   */     {'.','>',0,0},
/*  55   */     {'/','?','/' | MODIFIER_NUMLK,0},
/*  56   */     {0,0,0,0},
/*  57   */     {KEYCODE_RSHIFT,0,0,0},
/*  58   */     {KEYCODE_LCTRL,0,KEYCODE_RCTRL,0},
/*  59   */     {0,0,0,0},
/*  60   */     {KEYCODE_ALT,0,KEYCODE_ALTGR,0},
/*  61   */     {' ',0,0,0},
/*  62   */     {KEYCODE_ALTGR,0,0,0},
/*  63   */     {0,0,0,0},
/*  64   */     {KEYCODE_RCTRL,0,0,0},
/*  65   */     {0,0,0,0},
/*  66   */     {0,0,0,0},
/*  67   */     {0,0,0,0},
/*  68   */     {0,0,0,0},
/*  69   */     {0,0,0,0},
/*  70   */     {0,0,0,0},
/*  71   */     {0,0,0,0},
/*  72   */     {0,0,0,0},
/*  73   */     {0,0,0,0},
/*  74   */     {0,0,0,0},
/*  75   */     {KEYCODE_INSERT,0,0,0},
/*  76   */     {KEYCODE_DELETE,0,0,0},
/*  77   */     {0,0,0,0},
/*  78   */     {0,0,0,0},
/*  79   */     {KEYCODE_LEFT,0,0,0},
/*  80   */     {KEYCODE_HOME,0,0,0},
/*  81   */     {KEYCODE_END,0,0,0},
/*  82   */     {0,0,0,0},
/*  83   */     {KEYCODE_UP,0,0,0},
/*  84   */     {KEYCODE_DOWN,0,0,0},
/*  85   */     {KEYCODE_PAGEUP,0,0,0},
/*  86   */     {KEYCODE_PAGEDN,0,0,0},
/*  87   */     {0,0,0,0},
/*  88   */     {0,0,0,0},
/*  89   */     {KEYCODE_RIGHT,0,0,0},
/*  90   */     {KEYCODE_NUMLK,0,KEYCODE_BREAK,0},
/*  91   */     {KEYCODE_HOME | MODIFIER_NUMLK,0,KEYCODE_HOME,0},
/*  92   */     {KEYCODE_LEFT | MODIFIER_NUMLK,0,KEYCODE_LEFT,0},
/*  93   */     {KEYCODE_END | MODIFIER_NUMLK,0,KEYCODE_END,0},
/*  94   */     {0,0,0,0},
/*  95   */     {'/' | MODIFIER_NUMLK,0,0,0},
/*  96   */     {KEYCODE_UP | MODIFIER_NUMLK,0,KEYCODE_UP,0},
/*  97   */     {KEYCODE_CENTER | MODIFIER_NUMLK,0,KEYCODE_CENTER,0},
/*  98   */     {KEYCODE_DOWN | MODIFIER_NUMLK,0,KEYCODE_DOWN,0},
/*  99   */     {KEYCODE_INSERT | MODIFIER_NUMLK,0,KEYCODE_INSERT,0},
/*  100  */     {'*' | MODIFIER_NUMLK,0,KEYCODE_PRTSCR,0},
/*  101  */     {KEYCODE_PAGEUP | MODIFIER_NUMLK,0,KEYCODE_PAGEUP,0},
/*  102  */     {KEYCODE_RIGHT | MODIFIER_NUMLK,0,KEYCODE_RIGHT,0},
/*  103  */     {KEYCODE_PAGEDN | MODIFIER_NUMLK,0,KEYCODE_PAGEDN,0},
/*  104  */     {KEYCODE_DELETE | MODIFIER_NUMLK,0,KEYCODE_DELETE,0},
/*  105  */     {'-' | MODIFIER_NUMLK,0,0,0},
/*  106  */     {'+' | MODIFIER_NUMLK,0,0,0},
/*  107  */     {KEYCODE_ENTER,0,0,0},
/*  108  */     {0,0,0,0},
/*  109  */     {0,0,0,0},
/*  110  */     {KEYCODE_ESCAPE,0,0,0},
/*  111  */     {0,0,0,0},
/*  112  */     {KEYCODE_F1,0,0,7},
/*  113  */     {KEYCODE_F2,0,0,0},
/*  114  */     {KEYCODE_F3,0,0,0},
/*  115  */     {KEYCODE_F4,0,0,0},
/*  116  */     {KEYCODE_F5,0,0,0},
/*  117  */     {KEYCODE_F6,0,0,0},
/*  118  */     {KEYCODE_F7,0,0,0},
/*  119  */     {KEYCODE_F8,0,0,0},
/*  120  */     {KEYCODE_F9,0,0,0},
/*  121  */     {KEYCODE_F10,0,0,0},
/*  122  */     {KEYCODE_F11,0,0,0},
/*  123  */     {KEYCODE_F12,0,0,0},
/*  124  */     {KEYCODE_PRTSCR,0,0,0},
/*  125  */     {KEYCODE_SCRLK,0,KEYCODE_BREAK,0},
/*  126  */     {KEYCODE_BREAK,0,0,0},
/*  127  */     {0,0,0,0},
/*  128  */     {KEYCODE_WINL,0,KEYCODE_WINL,0},
/*  129  */     {KEYCODE_WINR,0,KEYCODE_WINR,0},
/*  130  */     {KEYCODE_MENU,0,KEYCODE_MENU,0},
/* currently no keys with numbers > 130 */
/*  131  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  140  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  150  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  160  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  170  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  180  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  190  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  200  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  210  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  220  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  230  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  240  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
/*  250  */     {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}
};
 
/*
 * Scan code to key number conversion table for
 * an extended AT keyboard in mode 2
 *
 * This will give us the key index for keyboard
 */
int scancode_mode2_extended[256] = {
  	0, 	120, 	0, 	116, 	114, 	112, 	113,
	123, 	0, 	121, 	119, 	117, 	115,	16,
	1, 	0, 	0, 	60, 	44, 	0, 	58,
	17, 	2, 	0, 	0, 	0, 	46, 	32,
	31, 	18,	3, 	128, 	0, 	48, 	47,
	33, 	19, 	5, 	4, 	129, 	0, 	61,
	49, 	34, 	21, 	20,	6, 	130, 	0,
	51, 	50, 	36, 	35, 	22, 	7, 	0,
	0, 	0, 	52, 	37, 	23, 	8, 	9,
	0, 	0, 	53, 	38, 	24, 	25, 	11,
	10, 	0, 	0, 	54, 	55, 	39, 	40,
	26, 	12, 	0, 	0, 	0, 	41, 	0,
	27, 	13, 	0, 	0, 	30, 	57, 	43,
	28, 	0, 	29, 	0, 	0, 	0, 	45,
	0, 	0, 	0, 	0, 	15, 	0, 	0,
	93, 	0, 	92, 	91, 	0, 	0, 	0,
	99, 	104,	98, 	97, 	102, 	96, 	110,
	90, 	122, 	106, 	103, 	105, 	100, 	101,
	125,  	0, 	0, 	0, 	0, 	118, 	0,
	0, 	0, 	0, 	0, 	0, 	0, 	0,
	0, 	0, 	0, 	0,
  	/* no keys with codes > 0x8F */
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
 
#endif /* __KEYMAP_H__ */
