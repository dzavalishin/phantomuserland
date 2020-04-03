/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Runnable. Place to hardcode run method ordinal.
 *
**/


package .ru.dz.phantom.system;

attribute const * ->!;

class runnable
{
    // They call us here
    void run(var parent_object @const ) [8]
    {
        throw "Abstract runnable started";
    }

};







