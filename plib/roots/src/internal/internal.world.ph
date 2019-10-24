/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * How to access world. Gives access to own thread object 
 * which, in turn, can give access to global OS services.
 *
 * TODO: directly access some really global things?
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

import .internal."thread";

class .internal.world
{

	.internal."thread" getMyThread() [8] {  }

};



