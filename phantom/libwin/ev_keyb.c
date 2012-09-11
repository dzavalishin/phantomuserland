
#include <threads.h>
#include <dev/key_event.h>

#include <event.h>
#include "ev_private.h"

static void ev_send_key_event_to_q(_key_event *event)
{

    static int shifts;

    int dn = event->modifiers & KEY_MODIFIER_DOWN;

    if( event->modifiers & KEY_MODIFIER_UP )
        shifts |= UI_MODIFIER_KEYUP;

    if( dn )
        shifts &= ~UI_MODIFIER_KEYUP;

    switch(event->keycode)
    {
    case KEY_LSHIFT:
        shifts &= ~UI_MODIFIER_LSHIFT;
        if(dn)
            shifts |= UI_MODIFIER_LSHIFT;

    set_comon_shift:
        shifts &= ~UI_MODIFIER_SHIFT;
        if( (shifts & UI_MODIFIER_LSHIFT) || (shifts & UI_MODIFIER_RSHIFT) )
            shifts |= UI_MODIFIER_SHIFT;
        break;

    case KEY_RSHIFT:
        shifts &= ~UI_MODIFIER_RSHIFT;
        if(dn)
            shifts |= UI_MODIFIER_RSHIFT;

        goto set_comon_shift;



    case KEY_LCTRL:
        shifts &= ~UI_MODIFIER_LCTRL;
        if(dn)
            shifts |= UI_MODIFIER_LCTRL;

    set_comon_ctrl:
        shifts &= ~UI_MODIFIER_CTRL;
        if( (shifts & UI_MODIFIER_LCTRL) || (shifts & UI_MODIFIER_RCTRL ) )
            shifts |= UI_MODIFIER_CTRL;
        break;

    case KEY_RCTRL:
        shifts &= ~UI_MODIFIER_RCTRL;
        if(dn)
            shifts |= UI_MODIFIER_RCTRL;

        goto set_comon_ctrl;



    case KEY_LALT:
        shifts &= ~UI_MODIFIER_LALT;
        if(dn)
            shifts |= UI_MODIFIER_LALT;

    set_comon_alt:
        shifts &= ~UI_MODIFIER_ALT;
        if( (shifts & UI_MODIFIER_LALT) || (shifts & UI_MODIFIER_RALT ) )
            shifts |= UI_MODIFIER_ALT;
        break;

    case KEY_RALT:
        shifts &= ~UI_MODIFIER_RALT;
        if(dn)
            shifts |= UI_MODIFIER_RALT;

        goto set_comon_alt;


    case KEY_LWIN:
        if(dn)              shifts |= UI_MODIFIER_LWIN;
        else             	shifts &= ~UI_MODIFIER_LWIN;

    set_comon_win:
        if( (shifts & UI_MODIFIER_LWIN) || (shifts & UI_MODIFIER_RWIN ) )
            shifts |= UI_MODIFIER_WIN;
        else
            shifts &= ~UI_MODIFIER_WIN;
        break;

    case KEY_RWIN:
        if(dn)              shifts |= UI_MODIFIER_RWIN;
        else                shifts &= ~UI_MODIFIER_RWIN;
        goto set_comon_win;




    case KEY_CAPSLOCK:
        if(dn)
        {
            if(shifts & UI_MODIFIER_CAPSLOCK)
                shifts &= ~UI_MODIFIER_CAPSLOCK;
            else
                shifts |= UI_MODIFIER_CAPSLOCK;
        }

    case KEY_SCRLOCK:
        if(dn)
        {
            if(shifts & UI_MODIFIER_SCRLOCK)
                shifts &= ~UI_MODIFIER_SCRLOCK;
            else
                shifts |= UI_MODIFIER_SCRLOCK;
        }

    case KEY_PAD_NUMLOCK:
        if(dn)
        {
            if(shifts & UI_MODIFIER_NUMLOCK)
                shifts &= ~UI_MODIFIER_NUMLOCK;
            else
                shifts |= UI_MODIFIER_NUMLOCK;
        }

    }

    ev_q_put_key( event->keycode, event->keychar, shifts );

}






void ev_keyboard_read_thread(void)
{
    t_current_set_name("KeyEvents");
    t_current_set_priority(PHANTOM_SYS_THREAD_PRIO);

    while(1)
    {
        _key_event ke;

        phantom_dev_keyboard_get_key( &ke );
        ev_send_key_event_to_q(&ke);
    }
}
