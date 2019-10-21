/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Phantom OS team
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Generic UI control: button, text field, menu, etc
 *
**/

#ifndef W_CONTROL_H
#define W_CONTROL_H

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H


#include <video/window.h>
#include <video/rect.h>
#include <kernel/pool.h>
#include <sys/types.h>
#include <stdint.h>

#define W_CONTROL_BS 64 // Size of text buffer iside a control struct - for save in persisten mem

// negative magic produces negative handles
//#define CONTROLS_POOL_MAGIC ('b' << 24 | 0xFFEEAA)
#define CONTROLS_POOL_MAGIC ('b')


// Control flags - lower 16 bits are generic (any control), 
// upper 16 bits are specific for controls
#define CONTROL_FLAG_DISABLED          (1<<0)  //< Not seen, does not process events
#define CONTROL_FLAG_NOPAINT           (1<<1)  //< Do not paint, but process mouse
#define CONTROL_FLAG_NOBORDER          (1<<2)  //< Do not paint border
#define CONTROL_FLAG_CALLBACK_HOVER    (1<<3)  //< Call callback on mouse over
#define CONTROL_FLAG_CALLBACK_KEY      (1<<4)  //< Call callback on any key press
#define CONTROL_FLAG_TOGGLE            (1<<5)  //< Button or menu item toggles
#define CONTROL_FLAG_HORIZONTAL        (1<<6)  //< Put children left to right - menu
#define CONTROL_FLAG_TEXT_RIGHT        (1<<7)  //< Put button text to the right of image
#define CONTROL_FLAG_NOFOCUS           (1<<8)  //< This control is passive - no events, no focus
// NB - unused - convert to NOBGFILL to turn off just painting rectangle with bg color?
#define CONTROL_FLAG_NOBACKGROUND      (1<<9)  //< Do not paint control background - TODO check me
#define CONTROL_FLAG_READONLY         (1<<10)  //< Control is just for display, don't accept user input - TODO implement
#define CONTROL_FLAG_ALT_FG           (1<<11)  //< Control foreground (active part) is paint in a different way - TODO
#define CONTROL_FLAG_ALT_BG           (1<<12)  //< Control background (passive part) is paint in a different way - TODO

// TODO - do we need CONTROL_FLAG_READONLY? Can use CONTROL_FLAG_NOFOCUS instead?

struct control;

/**
 * 
 * Group of controls, used for menu items and radiobuttons.
 * 
** /
typedef struct 
{
    struct control  *siblings;
} control_group_t;
*/


/// Type of visual control - button, text box, etc
typedef enum {
    ct_unknown     = 0,
    ct_button      = 1,
    ct_text        = 2, // text field
    ct_menu        = 3,
    ct_menuitem    = 4,
    ct_label       = 5,
    ct_scrollbar   = 6,

    //ct_group       = 0xFF+0,            //< To store it in an .internal.ui.control we put group in a control union too.
} control_type_t;


/// Current state of control
typedef enum {
    cs_default     = 0,
    cs_released    = 1,
    //cs_hover       = 2,
    cs_pressed     = 3,
} control_state_t;

/// Current state of control
typedef enum {
    ch_normal      = 0,
    ch_hover       = 1,
} hover_state_t; 


typedef pool_handle_t control_handle_t;

// Take callback rg directly from cc
typedef void (*control_callback_t)( window_handle_t w, struct control *cc );

/**
 * 
 * \brief General UI control definition.
 * 
 * TODO gaps? packed?
 * 
**/
typedef struct control
{
    control_type_t      type;           //< Type of this control
    u_int32_t           id;             //< Identifier of this control - sent in messages from it
    u_int32_t           group_id;       //< If not zero, all controls with same group_id will be brought to same group

    u_int32_t           flags;          //< See above

    rect_t              r;              //< Where to paint in window
    color_t             bg_color;       //< Backgroud color to use, where applicable
    color_t             fg_color;       //< Foreground color to use, where applicable

    // TODO image handle!
    drv_video_bitmap_t *pas_bg_image;   //< Passive (non-pressed) background image
    drv_video_bitmap_t *act_bg_image;   //< Active (pressed) background image
    drv_video_bitmap_t *hov_bg_image;   //< Mouse over background image

    drv_video_bitmap_t *icon_image;     //< Icon image - text is put aside

    const char *        text;           //< Text, if applicable. Will be copied.
    //color_t             text_color;     //< Text color

    control_callback_t  callback;       //< To call if state changed
    union {
        void *          callback_arg;   //< User arg to callback
        void *          callback_arg_p; //< User arg to callback
        int             callback_arg_i;
    };

    control_handle_t    c_child;        //< Activate child on state == cs_pressed - usually submenu
    window_handle_t     w_child;        //< Activate on state == cs_pressed

    window_handle_t     context_menu;   //< Show on right click

    union {
        uint8_t         _space[64];     //< To provide for growth

        /*struct {
            uint32_t    _unused_btn;
        };*/

        struct {
            uint32_t    maxlen;         //< Max __BYTES__ of text. Note UTF8 takes more. If o - use W_CONTROL_BS
        };                              //< Specific for text field

        struct {
            uint32_t    vkey;           //< Hotkey for menu item
        };                              //< Specific for menu item

        struct { // text field
            int32_t     vis_shift;      //< First character we see in edit window
            int32_t     vis_len;        //< Num of chars we see in window
            int32_t     str_len;        //< Num of bytes in buffer
            int32_t     cursor_pos;     //< Cursor position in bytes - TODO UTF-32!
            int32_t     text_height;    //< Rendered text height in pixels, max possible
        };

        struct { // scroll bar
            int32_t     minval;         //< Max value
            int32_t     maxval;         //< Max value
            int32_t     value;          //< Cur value
            int32_t     value_width;    //< Size of handle = value_width / (maxval - minval)
        };

    };

    // -------------------------------------------------------------------
    //
    // Private part, passed values will be ignored / reset up
    //
    // -------------------------------------------------------------------

    window_handle_t     w;              //< My window

    control_state_t     state;          //< State - for display and action
    hover_state_t       hovered;        //< Hovered by mouse
    uint32_t            focused;        //< Selected in window
    uint32_t            changed;        //< Needs repaint
    uint32_t            notify;         //< Notifications count - round bullet with number in the top right corner

    //control_group_t *   group;          //< Group we belong, if any
    //struct control *    next_in_group;  //< linked list of controls in group - radio or menu

    uint32_t            pas_bg_alloc;   //< Must free on finish or reload
    uint32_t            act_bg_alloc;   //< Must free on finish or reload
    uint32_t            hov_bg_alloc;   //< Must free on finish or reload

    char                buffer[W_CONTROL_BS];       //< if in persistent memory, text field text is here

} control_t;



/// This is what we keep in pool actually, to be able to
/// hold controls in persistent mem.
typedef struct
{
    control_t         * c;              //< Control
} control_ref_t;




// -----------------------------------------------------------------------
//
// Public interface
//
// -----------------------------------------------------------------------


/**
 * 
 * \brief Add control to window.
 * 
 * Structure passed is not stored but copied. You can modify it
 * and pass again for next control.
 * 
 * \param[in] w Window to add to
 * \param[in] c control_t structure to copy control properties from
 * 
 * \return Handle for a control
**/ 
control_handle_t w_add_control( window_handle_t w, control_t *c );

/// Same as w_add_control(), but you can pass list of controls linked by next_in_group field.
//void w_add_controls( window_handle_t w, control_t *c );

control_handle_t w_get_control_by_id( window_handle_t w, uint32_t id ); // To be able to access mass added controls

/// Same as w_add_control(), but actually USES given control_t structure, which supposed to live in persistent storage.
pool_handle_t w_add_control_persistent( window_handle_t w, control_t *c );

/// Same as w_add_control_persistent(), but assumes we are restarting and tries to use internal state too.
pool_handle_t w_restart_control_persistent( window_handle_t w, control_t *c );

void w_clear_control( control_t *c ); //< Prepare structure to fill field by field


void w_delete_control( window_handle_t w, control_handle_t c );

//! UTF-8
void w_control_set_text( window_handle_t w, control_handle_t c, const char *text, color_t text_color );
//! UTF-8
void w_control_get_text( window_handle_t w, control_handle_t c, char *text_buf, size_t buf_size );
void w_control_set_icon( window_handle_t w, control_handle_t ch, drv_video_bitmap_t *icon );

//! NB! Allocates new bitmaps and does alpha blending with basic window background
void w_control_set_background( window_handle_t w, control_handle_t ch, 
    drv_video_bitmap_t *normal, drv_video_bitmap_t *pressed, drv_video_bitmap_t *hover  );


void w_control_set_callback( window_handle_t w, control_handle_t c, control_callback_t cb, void *callback_arg );
/// If just window is given, switch its visibility. If window and control - switch control visibility.
void w_control_set_children( window_handle_t w, control_handle_t c, window_handle_t w_child, control_handle_t c_child );
void w_control_set_visible( window_handle_t w, control_handle_t ch, int visible ); // unimpl yet
void w_control_set_flags( window_handle_t w, control_handle_t ch, int toSet, int toReset );
void w_control_set_position( window_handle_t w, control_handle_t ch, int x, int y );
void w_control_set_notify( window_handle_t w, control_handle_t ch, int count ); //< show bullet with number in top right corner
void w_control_set_menu( window_handle_t w, control_handle_t ch, window_handle_t m ); //< set context (right click) menu

void w_control_set_state( window_handle_t w, control_handle_t ch, int pressed ); //< Is checkbox checked or switch turned on?
void w_control_get_state( window_handle_t w, control_handle_t ch, int *ret ); //< Is checkbox checked or switch turned on?

/**
 * 
 * Control scrollbar position ad size.
 * 
 * \param[in] value Current value. Determines start position of scroll bar.
 * \param[in] width Width of scroll bar - how big part of possible values it takes. If == maxval - minval - takes 100% of scrollbar size
 * 
 * Set value or width to be negative to disable display of bar at all.
 * 
**/
void w_control_set_value( window_handle_t w, control_handle_t ch, int value, int width );  //< For scrollbar - set value & bar handle width
void w_control_get_value( window_handle_t w, control_handle_t ch, int *value, int *width ); //< For scrollbar - get value & bar handle width

// -----------------------------------------------------------------------
//
// Control creation: Shortcuts for typical cases
//
// -----------------------------------------------------------------------


control_handle_t w_add_button( window_handle_t w, int id, int x, int y, drv_video_bitmap_t *bmp, drv_video_bitmap_t *pressed, int flags );

control_handle_t w_add_radio_button( window_handle_t w, int id, int group_id, int x, int y );

control_handle_t w_add_checkbox( window_handle_t w, int x, int y );

control_handle_t w_add_menu_item( window_handle_t w, int id, int x, int y, int xsize, const char*text, color_t text_color );

control_handle_t w_add_label( window_handle_t w, int x, int y, int xsize, int ysize, const char *text, color_t text_color );
control_handle_t w_add_label_transparent( window_handle_t w, int x, int y, int xsize, int ysize, const char *text, color_t text_color );
control_handle_t w_add_label_ext( window_handle_t w, int x, int y, int xsize, int ysize, const char *text, color_t text_color, 
                                  drv_video_bitmap_t *bg, uint32_t flags );

control_handle_t w_add_text_field( window_handle_t w, int x, int y, int xsize, int ysize, const char *text, color_t text_color );

control_handle_t w_add_scrollbar_ext( window_handle_t w, int x, int y, int xsize, int ysize, int minval, int maxval, uint32_t flags );
control_handle_t w_add_scrollbar( window_handle_t w, int x, int y, int xsize, int ysize, int maxval );


// -----------------------------------------------------------------------
//
// Private windows system controls management funcs
//
// -----------------------------------------------------------------------



pool_t *create_controls_pool(void);
void destroy_controls_pool(pool_t *controls);


void w_paint_changed_controls(window_handle_t w);
void w_repaint_controls(window_handle_t w);
void w_reset_controls(window_handle_t w); // focus lost, mouse off window - make sure all buttons are off -- TODO used?
int w_event_to_controls( window_handle_t w, ui_event_t *e ); // Deliver events to controls - return nonzero if event consumed

void w_paint_control( window_handle_t w, control_t *cc );

/**
 * 
 * Delete character from control's buffer at given position.
 * 
**/
void w_ctl_delete_buffer_char( control_t *cc, int pos );


// -------------------------------------------------------------------
//
// Menu window
//
// -------------------------------------------------------------------

window_handle_t w_create_menu_window( int x, int y, int xsize, int ysize);



#endif // W_CONTROL_H
