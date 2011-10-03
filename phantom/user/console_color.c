#include <video/color.h>

void console_set_fg_color( struct rgba_t fg ) 
{
    (void) fg;
}

int debug_max_level_error = 10;
int debug_max_level_info = 10;
int debug_max_level_flow = 10;
