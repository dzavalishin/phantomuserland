/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * IO - connection to Unix personality, file systems, devices and so on. 
 *
 *
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will never load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

package .internal;


class io
{
	void    open( var filename : string ) [8] {}
	void	close() [9] {}

	void	read( var nBytes : int ) [10] {}
	void    write( var data : string ) [11] {}

	void    seek( var pos: int, var whence: int ) [16] {}

	// ioctl
	//void	setAttribute( var name : string, var value : string ) [12] {}
	//void	getAttribute( var name : string ) [13] {}

    // TODO
	//void	setData( var name : string, var value : string ) [12] {}
	//void	getData( var name : string ) [13] {}

};
