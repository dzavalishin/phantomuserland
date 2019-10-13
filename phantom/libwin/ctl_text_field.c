/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * UI controls: Text field
 *
**/

#define DEBUG_MSG_PREFIX "ui.ctl.txt"
#include <debug_ext.h>
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 1


#include <phantom_types.h>
#include <phantom_libc.h>
#include <assert.h>

#include <kernel/pool.h>

#include <video/rect.h>
#include <video/window.h>
#include <video/bitmap.h>
#include <video/font.h>
#include <video/internal.h>

#include <video/control.h>

#include <dev/key_event.h>

#include "ctl_private.h"


// -----------------------------------------------------------------------
//
// Local helpers
//
// -----------------------------------------------------------------------


static void ctl_text_shift_for_cursor( control_t *cc )
{
    if( cc->cursor_shift < cc->vis_shift )
        cc->vis_shift = cc->cursor_shift;

    if( cc->cursor_shift > (cc->vis_shift + cc->vis_len) )
        cc->vis_shift = cc->cursor_shift - cc->vis_len;
}

// TODO rewrite all this in UTF code points, not bytes!
// Add ttf render func for UTF

static void find_visible_text_len( control_t *cc )
{
    int vis_width_pixels = 0;
    // TODO we must cut off not byte but UTF-8 char
    while(1) {
        rect_t r;

        if( cc->vis_len > cc->str_len - cc->vis_shift )
            cc->vis_len = cc->str_len - cc->vis_shift;

        w_ttfont_string_size( decorations_title_font,
                          cc->buffer + cc->vis_shift, cc->vis_len,
                           &r );

        if (vis_width_pixels <= cc->r.xsize - 5 )
            break;

        // sanity
        if( cc->vis_len <= 1 )
            break;
    }     
}



// -----------------------------------------------------------------------
//
// Main code
//
// -----------------------------------------------------------------------


void ctl_text_field_paint(window_handle_t win, control_t *cc )
{
    ctl_paint_bg( win, cc );    
    
    int t_height = 16;
    int t_ypos = (cc->r.ysize - t_height) / 2;

    ctl_text_shift_for_cursor( cc );
    find_visible_text_len( cc );
    ctl_text_shift_for_cursor( cc ); // Width could change

    int cursor_x_pos = 0;

    w_ttfont_draw_string_ext( win, decorations_title_font,
                          cc->buffer+cc->vis_shift, cc->vis_len,
                          cc->fg_color,
                          cc->r.x + t_ypos + 2, cc->r.y+t_ypos,
                          &cursor_x_pos, cc->cursor_shift ); // +2?
    
    // Cursor
    //if(cc->focused)
    {
    //w_draw_v_line( win, COLOR_GREEN, cursor_x_pos, cc->r.y, cc->r.ysize );
        w_draw_line( win, cursor_x_pos - 1, cc->r.y + 2, cursor_x_pos - 1, cc->r.y+cc->r.ysize - 4, COLOR_DARKGRAY );
    }

    ctl_paint_border( win, cc );
    //LOG_FLOW( 10, "paint label id %d", cc->id );
}






/// Process event for text edit field
int ctl_text_field_events(control_t *cc, struct foreach_control_param *env)
{
    ui_event_t e = env->e;

    // TODO need to check if current control is focused
    //if( !cc->focused ) return;

    if( (e.type == UI_EVENT_TYPE_KEY) && UI_MOD_DOWN(e.modifiers) )
    {
        LOG_FLOW( 1, "key ev vk=%d, ch=%d", e.k.vk, e.k.ch );
        cc->changed = 1;
        switch (e.k.ch)
        {
        case KEY_RIGHT:
            cc->cursor_shift++;

check_cursor_right:
            if( cc->cursor_shift > cc->str_len )
                cc->cursor_shift = cc->str_len;
            break;

        case KEY_LEFT:
            cc->cursor_shift--;
            if( cc->cursor_shift < 0 )                
                cc->cursor_shift = 0;
            break;

        case KEY_HOME:
            cc->cursor_shift = 0;
            break;

        case KEY_END:
            cc->cursor_shift = cc->str_len;
            break;
        case KEY_BACKSPACE:
            if(cc->cursor_shift <= 0) break;
            cc->cursor_shift--;
            /* FALLTHROUGH */
        case KEY_DELETE:
            if( cc->cursor_shift >= cc->str_len )
                break;
            {
                const char *from = cc->buffer + cc->cursor_shift + 1;
                char *to = cc->buffer + cc->cursor_shift;
                size_t len = cc->str_len - (cc->cursor_shift + 1);
                strncpy( to, from, len );
            }
            break;

        default:
            cc->changed = 0;

            if( !KEY_IS_FUNC(e.k.ch) )
            {
                cc->changed = 1;

                if( cc->cursor_shift >= sizeof(cc->buffer)-1 )
                    break;

                char *from = cc->buffer + cc->cursor_shift;
                char *to = cc->buffer + cc->cursor_shift + 1;
                size_t len = cc->str_len - (cc->cursor_shift + 1);
                // if(insert_mode)
                //strncpy( to, from, len );
                memmove( to, from, len );

                *from = e.k.ch;
                if( cc->str_len == cc->cursor_shift) // extend line
                {
                    cc->str_len++;
                    if( cc->str_len >= sizeof(cc->buffer)-1 )
                        cc->str_len = sizeof(cc->buffer)-1;
                }

                cc->cursor_shift++;
                goto check_cursor_right;
            }

            break;
        }
        
        LOG_FLOW( 1, "cursor pos = %d", cc->cursor_shift );
        return 1; // eat event
    }

    return 0; // do not eat event
}
