/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal: Yes
 * Preliminary: Yes
 *
 *
**/

package .internal;

/**
 *
 * Object of this class is synthesized by kernel on OS
 * instance creation. Very early and special kernel
 * interface methods.
 *
**/

import .phantom.osimpl;
import .internal.binary;
import .internal.bitmap;
import .internal."class";

class bootstarap
{
    /**
     *
     * The very first class loader. Not guaranteed to be working
     * on the second OS restart. Supposed to be used during OS
     * instance setup only. Currently loads from module passed
     * to kernel with multiboot bootloader.
     *
    **/
    .internal."class" loadClass( var name : .internal.string ) {8] {}

    void consolePrint( var s : .internal.string ) [16] {}

    void setClassLoader( var cl : .internal.object ) [17] {}

    /**
     *
     * Runs method 8 of object in a new thread. Object
     * supposed to be .phantom.runnable, FIXME.
     *
     * Note that all such threads will start only after the root
     * OS init code is finished.
     *
    **/
    void startThread( var o : .internal.object ) [18] {}

    .internal.binary createBinary( var sizeBytes : int ) [19] {}

    void setScreenBackground( var bg : .internal.bitmap ) [20] {}

    // TODO kill this!
    void sleep( var msec : int ) [21] {}

    /**
     *
     * This interface will be available globally to all threads,
     * if not replaced specifically.
     *
    **/

    void setOsInterface(var oi : .phantom.osimpl ) [22] {}

    /**
     *
     * Returns kernel environment - array of strings
     * containing 'name=value' style information about
     * kernel and it's stuff.
     *
    **/

    .internal.string [] getKernelEnvironment() [23] {}

};

