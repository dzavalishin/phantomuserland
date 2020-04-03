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
 * .internal package). It means that VM will bever load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

/**
 *
 * Binary object - just an array of bytes. Used as a container for
 * bitmaps, for example.
 *
 * TODO: Must be dynamically resizeable.
 *
**/

class binary
{
	int	getByte( var index : int ) [8] {}
	void	setByte( var index : int, var value : int ) [9] {}

	void	setRange( var from : .internal.binary, var toPos : int, var fromPos : int, var ln : int ) [10] {}
};
