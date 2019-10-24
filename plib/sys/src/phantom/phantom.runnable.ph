/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Runnable. Place to hardcode run method ordinal.
 *
**/


package .phantom;

attribute const * ->!;

class runnable
{
    // They call us here
    void run( var arg @const ) [8]
    {
        throw "Abstract runnable started";
    }

};







