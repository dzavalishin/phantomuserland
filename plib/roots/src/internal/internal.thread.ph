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

import .phantom.osimpl;
import .phantom.user;
import .phantom.environment;

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
 * Thread. 
 *
**/

class .internal."thread"
{

	.internal.string toString() [5] { }

        /** pause thread */
        void pause() [10] {}

        /** let thread run again */
        void unpause() [11] {}

        /**
         *
         * Return (possibly specific for this thread) OS
         * interface object, wchich is used to access public
         * OS services.
         *
         * @See .phantom.osimpl
         *
        **/
        .phantom.osimpl getOsInterface() [14] { }

        /**
         *
         * Return owner of this thread. For the root OS
         * threads this can be null.
         *
        **/
        .phantom.user getUser() [13] {}

        /**
         *
         * Return this thread's environment (Unix-style).
         *
        **/
        .phantom.environment getEnvironment() [12] {}
};



