/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal: yes
 * Preliminary: yes
 *
 *
**/

package .internal;

/**
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will never load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

/**
 *
 * Bitmap - RGBA + size;
 *
 * TODO: Getsize, load from binary, more formats to parse.
 *
**/

import .internal.io.tty;

class .internal.bitmap
{
    // Parse PPM (P6) bitmap file. Alpha set to 255.
    void loadFromString(var src : string) [8] {}

    // That's temp - define some 'paintable' interface?
    void paintTo( var window : .internal.io.tty, var xpos : int, var ypos : int ) [9] {}
};

