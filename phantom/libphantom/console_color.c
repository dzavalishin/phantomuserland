/**
 *
 * Phantom OS
 *
 * Console colors definitions.
 *
 * (C) 2005-2016 dz.
 *
**/



#include <console.h>

void    console_set_message_color()     { console_set_fg_color( COLOR_LIGHTGREEN ); }
void    console_set_error_color()       { console_set_fg_color( COLOR_LIGHTRED ); }
void    console_set_warning_color()     { console_set_fg_color( COLOR_YELLOW ); }
void    console_set_normal_color()      { console_set_fg_color( COLOR_LIGHTGRAY ); }
