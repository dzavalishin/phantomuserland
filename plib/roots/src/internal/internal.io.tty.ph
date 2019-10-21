/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * TTY style window. 
 *
 * TTY - temporary screen/keyb io. To be redone.
 *
 * Not really of big use. Please prefer .internal.window or
 * warppers around it.
 *
 * It is not just a tty, as you can blit a bitmap to it. See bitmap class.
 *
 * TODO: getwc is temp off
 *
**/

package .internal.io;

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
 *
**/

class tty 
{
	void	debug(var o : .internal.object ) [18] {}

	string	getwc() [16] {} 

	void    putws( var text : string ) [17] {}
	void    gotoxy( var x: int, var y: int ) [19] {}
	void	clear() [20] {}

	void	setcolor( var color : int ) [21] {}
	void	setbgcolor( var color : int ) [26] {}

	void	moveWindow( var x : int, var y : int ) [24] {}
	void	setTitle( var title : string ) [25] {}

};
