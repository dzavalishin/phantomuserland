/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Phantom OS team
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * UI controls private header.
 *
**/

#ifndef W_CONTROL_PRIVATE_H
#define W_CONTROL_PRIVATE_H

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H


// -------------------------------------------------------------------
//
// Text field control specific processing
//
// -------------------------------------------------------------------


int ctl_text_field_events(control_t *cc, struct foreach_control_param *env);
void ctl_text_field_paint(window_handle_t win, control_t *cc );

// -------------------------------------------------------------------
//
// Bits to combine in control paint function
//
// -------------------------------------------------------------------

void ctl_paint_bg( window_handle_t win, control_t *cc );
void ctl_paint_border( window_handle_t win, control_t *cc );
void ctl_paint_text( window_handle_t win, control_t *cc, int shift );
/// \return X shift for text
int ctl_paint_icon( window_handle_t win, control_t *cc );




#endif // W_CONTROL_PRIVATE_H
