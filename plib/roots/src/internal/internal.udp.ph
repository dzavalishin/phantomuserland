/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel UDP class
 *
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will never load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

package .internal;



class .internal.udp
{
	.internal.string bind( var port : .internal.int ) [16] {  }

	.internal.string recvFrom( var addr : .internal.int, var port : .internal.int ) [23] {  }

	.internal.int sendTo( var data : .internal.string, var addr : .internal.int, var port : .internal.int ) [25] {  }

};



