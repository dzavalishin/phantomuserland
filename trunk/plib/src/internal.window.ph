/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
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
 * OS level window object.
 * TODO: Must be dynamically resizeable.
 *
**/

class binary
{
	int	getXSize() [16] {}
	int	getYSize() [17] {}

	int	getX() [18] {}
	int	getY() [19] {}

	void	setSize( var xsize : int, var ysize : int ) [20] {}
	void	setPosition( var x : int, var y : int ) [21] {}

	void 	clear() [22] {}

	void	setBg( var bg : int ) [23] {}
	void	setFg( var bg : int ) [24] {}
 
	void	drawLine( var x : int, var y : int, var xsize : int, var ysize : int ) [25] {}
	void	drawBox( var x : int, var y : int, var xsize : int, var ysize : int ) [26] {}

	void	fillBox( var x : int, var y : int, var xsize : int, var ysize : int ) [27] {}
	void	fillEllipse( var x : int, var y : int, var xsize : int, var ysize : int ) [28] {}

	// todo will need font parameter
	void	drawString( var x : int, var y : int, var s : string ) [29] {}



};
