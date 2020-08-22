/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel UDP class
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


class .internal.udp
{

	.internal.string recvFrom( var addr : .internal.int ) [23] {  }

	.internal.int sendTo( var addr : .internal.int, var data : .internal.string ) [25] {  }

};



