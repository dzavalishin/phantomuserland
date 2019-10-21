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
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


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

//#include <dev/key_event.h>

#include "ctl_private.h"


// -----------------------------------------------------------------------
//
// Local helpers
//
// -----------------------------------------------------------------------

static void ctl_text_modify_shift( control_t *cc )
{
    LOG_FLOW( 1, "Cursor pos = %d, str_len = %d, vis_shift  = %d, vis_len = %d, buf='%s'", 
    cc->cursor_pos, cc->str_len, cc->vis_shift, cc->vis_len, cc->buffer );

    if( cc->cursor_pos > (cc->vis_shift + cc->vis_len ) )
        cc->vis_shift = cc->cursor_pos - cc->vis_len;

    if( cc->cursor_pos < cc->vis_shift )
        cc->vis_shift = cc->cursor_pos;

}


static void ctl_text_clip_state( control_t *cc )
{
    LOG_FLOW( 1, "Cursor pos = %d, str_len = %d, vis_shift  = %d, vis_len = %d, buf='%s'", 
    cc->cursor_pos, cc->str_len, cc->vis_shift, cc->vis_len, cc->buffer );

    if( cc->str_len < 0 )
    {
        LOG_ERROR( 1, "str_len = %d, < 0", cc->str_len );
        cc->str_len = 0;
    }

    if( cc->str_len >= sizeof(cc->buffer) )
    {
        LOG_ERROR( 1, "str_len = %d, >= sizeof(cc->buffer)", cc->str_len );
        cc->str_len = sizeof(cc->buffer) - 1;
    }

    if( cc->vis_shift >= cc->str_len )
    {
        LOG_ERROR( 1, "vis_shift = %d, >= cc->str_len", cc->vis_shift );
        cc->vis_shift = cc->str_len - 1;
    }

    if( cc->vis_shift < 0 )
    {
        LOG_ERROR( 1, "vis_shift = %d, < 0", cc->vis_shift );
        cc->vis_shift = 0;
    }


    if( cc->cursor_pos < 0 )
    {
        LOG_ERROR( 1, "Cursor pos = %d, < 0", cc->cursor_pos );
        cc->cursor_pos = 0;
    }

    if( cc->cursor_pos > cc->str_len )
    {
        LOG_ERROR( 1, "Cursor pos = %d, > cc->str_len", cc->cursor_pos );
        cc->cursor_pos = cc->str_len;
    }


    // Clip again
    //if(cc->vis_shift < 0) cc->vis_shift = 0;

    //if( cc->vis_shift >= cc->str_len )
    //    cc->vis_shift = cc->str_len - 1;

    if( cc->vis_len > cc->str_len - cc->vis_shift )
        cc->vis_len = cc->str_len - cc->vis_shift;

    if( cc->vis_len < 0 )
        cc->vis_len = 0;
}

// TODO rewrite all this in UTF code points, not bytes!
// Add ttf render func for UTF

static void find_visible_text_len( control_t *cc )
{
    rect_t r = { 0, 0, 0, 0 };

    // start from max possible
    cc->vis_len = cc->str_len - cc->vis_shift;

    // TODO we must cut off not byte but UTF-8 char
    while(1) {

//        if( cc->vis_len > cc->str_len - cc->vis_shift )
//            cc->vis_len = cc->str_len - cc->vis_shift;
        LOG_FLOW(4, "try vis_len %d ", cc->vis_len );

        w_ttfont_string_size( decorations_title_font,
                          cc->buffer + cc->vis_shift, cc->vis_len,
                           &r );

        int vis_width_pixels = r.xsize + 5; // TODO hack - +5 is to move away the right decoration
        LOG_FLOW(4, " vis_width_pixels %d, cc->r.xsize %d ", vis_width_pixels, cc->r.xsize );
        if (vis_width_pixels <= cc->r.xsize - 6 )
            break;

        cc->vis_len--;

        // sanity
        if( cc->vis_len <= 1 )
            break;
    }     

    //cc->text_height = r.ysize;
    if( cc->text_height <= 0 )
    {
        w_ttfont_string_size( decorations_title_font,
                          cc->buffer, cc->str_len,
                           &r );
        cc->text_height = r.ysize;
    }
}


// -----------------------------------------------------------------------
//
// Buffer processing
//
// -----------------------------------------------------------------------

void w_ctl_delete_buffer_char( control_t *cc, int pos )
{
    if( pos >= cc->str_len )
        return;
    
    LOG_FLOW( 2, "Delete ch @%d buf='%s'", pos, cc->buffer );

    const char *from = cc->buffer + pos + 1;
    char *to = cc->buffer + pos;
    size_t len = cc->str_len - (pos + 1);

    strncpy( to, from, len );

    assert(cc->str_len > 0);

    cc->str_len--;
    cc->buffer[cc->str_len] = 0;

    LOG_FLOW( 2, "Delete ch done @%d buf='%s'", pos, cc->buffer );
}


// -----------------------------------------------------------------------
//
// Main code
//
// -----------------------------------------------------------------------


void ctl_text_field_paint(window_handle_t win, control_t *cc )
{
    ctl_paint_bg( win, cc );    
    
    ctl_text_clip_state( cc );
    ctl_text_modify_shift( cc );
    ctl_text_clip_state( cc );
    find_visible_text_len( cc );
    ctl_text_clip_state( cc ); // Width could change

    int t_height = cc->text_height; //16;
    int t_ypos = (cc->r.ysize - t_height) / 2;

    int cursor_x_pos = 0;

    LOG_FLOW( 1, "Cursor pos = %d, str_len = %d, vis_shift  = %d, vis_len = %d, buf='%s'", 
    cc->cursor_pos, cc->str_len, cc->vis_shift, cc->vis_len, cc->buffer );
        
    w_ttfont_draw_string_ext( win, decorations_title_font,
                          cc->buffer+cc->vis_shift, cc->vis_len,
                          cc->fg_color,
                          cc->r.x + t_ypos + 2, cc->r.y+t_ypos,
                          &cursor_x_pos, cc->cursor_pos - cc->vis_shift ); // +2?
    
    // Cursor
    if(cc->focused || (cc->hovered == ch_hover))
    {
    //w_draw_v_line( win, COLOR_GREEN, cursor_x_pos, cc->r.y, cc->r.ysize );
        w_draw_line( win, cursor_x_pos - 1, cc->r.y + 3, cursor_x_pos - 1, cc->r.y+cc->r.ysize - 6, 
        COLOR_DARKGRAY );
        //cc->focused ? COLOR_GREEN : COLOR_LIGHTGRAY );
    }

    ctl_paint_border( win, cc );
    //LOG_FLOW( 10, "paint label id %d", cc->id );
}





/// Process event for text edit field
int ctl_text_field_events(control_t *cc, struct foreach_control_param *env)
{
    ui_event_t e = env->e;

    // Not focused - ignore key events
    if( !cc->focused ) return 0;

    if( (e.type == UI_EVENT_TYPE_KEY) && UI_MOD_DOWN(e.modifiers) )
    {
        LOG_FLOW( 3, "key ev vk=%d, ch=%d, buf='%s'", e.k.vk, e.k.ch, cc->buffer );
        cc->changed = 1;
        switch (e.k.ch)
        {
        case KEY_ENTER:
            //w_control_action(env->w, cc, &e); // see below
            break;

        case KEY_RIGHT:
            cc->cursor_pos++;

check_cursor_right:
            if( cc->cursor_pos > cc->str_len )
                cc->cursor_pos = cc->str_len;
            break;

        case KEY_LEFT:
            cc->cursor_pos--;
            if( cc->cursor_pos < 0 )                
                cc->cursor_pos = 0;
            break;

        case KEY_HOME:
            cc->cursor_pos = 0;
            break;

        case KEY_END:
            cc->cursor_pos = cc->str_len;
            break;

        case KEY_BACKSPACE:
            if(cc->cursor_pos <= 0) break;
            cc->cursor_pos--;
            /* FALLTHROUGH */
        case KEY_DELETE:
#if 1
            w_ctl_delete_buffer_char( cc, cc->cursor_pos );
#else
            if( cc->cursor_pos >= cc->str_len )
                break;
            {
                LOG_FLOW( 2, "Delete ch @%d buf='%s'", cc->cursor_pos, cc->buffer );
                const char *from = cc->buffer + cc->cursor_pos + 1;
                char *to = cc->buffer + cc->cursor_pos;
                size_t len = cc->str_len - (cc->cursor_pos + 1);
                strncpy( to, from, len );
                assert(cc->str_len > 0);
                cc->str_len--;
                cc->buffer[cc->str_len] = 0;
                LOG_FLOW( 2, "Delete ch done @%d buf='%s'", cc->cursor_pos, cc->buffer );
            }
#endif            
            break;

        default:
            cc->changed = 0;

            if( !KEY_IS_FUNC(e.k.ch) )
            {
                cc->changed = 1;

                LOG_FLOW( 2, "Insert ch '%c' (%d) buf = '%s'", e.k.ch, e.k.ch, cc->buffer );
                LOG_FLOW( 1, "Cursor pos = %d, str_len = %d, vis_shift  = %d, vis_len = %d", 
                    cc->cursor_pos, cc->str_len, cc->vis_shift, cc->vis_len );

                if( cc->cursor_pos >= sizeof(cc->buffer)-1 )
                    break;

                assert(cc->str_len < (sizeof(cc->buffer)-1));

                char *from = cc->buffer + cc->cursor_pos;
                char *to = cc->buffer + cc->cursor_pos + 1;
                //ssize_t len = cc->str_len - (cc->cursor_pos + 1);
                ssize_t len = cc->str_len - cc->cursor_pos;
                // if(insert_mode)
                if( (cc->str_len > 0) && (cc->cursor_pos < cc->str_len) ) // Can't extend empty string
                {
                    assert( len > 0 );
                    memmove( to, from, len );
                }

                *from = e.k.ch;

                cc->str_len++;
                if( cc->str_len >= sizeof(cc->buffer)-1 )
                    cc->str_len = sizeof(cc->buffer)-1;
                cc->buffer[cc->str_len] = 0;

                cc->cursor_pos++;
                LOG_FLOW( 2, "Insert ch done buf = '%s'", cc->buffer );
                goto check_cursor_right;
            }

            break;
        }
        
        LOG_FLOW( 9, "cursor pos = %d", cc->cursor_pos );

        if( (e.k.ch == KEY_ENTER) || (cc->flags & CONTROL_FLAG_CALLBACK_KEY) )
            w_control_action(env->w, cc, &e);

        return 1; // eat event
    }

    return 0; // do not eat event
}

