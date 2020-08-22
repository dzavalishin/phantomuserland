/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel debugg window support
 *
 *
**/

#ifndef KERNEL_DEBUG_GRAPHICAL_H
#define KERNEL_DEBUG_GRAPHICAL_H

#include <video/window.h> // for debug paint
#include <kernel/physalloc.h>

void paint_allocator_memory_map(window_handle_t w, rect_t *r, physalloc_t *m );

void paint_vaspace_allocator_memory_map(window_handle_t w, rect_t *r );
void paint_physmem_allocator_memory_map(window_handle_t w, rect_t *r );

void paint_persistent_memory_map(window_handle_t w, rect_t *r );
void paint_object_memory_map(window_handle_t w, rect_t *r );


#endif // KERNEL_DEBUG_GRAPHICAL_H
