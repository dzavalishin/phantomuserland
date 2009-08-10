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
 * TTY - temporary screen/keyb io. To be redone.
 * It is not just a tty, as you can blit a bitmap to it. See bitmap class.
 *
 * TODO: getwc blocks in kernel, preventing it from snaps!
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


};
