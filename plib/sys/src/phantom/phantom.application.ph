/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Application interface. Objects that implement this interface can 
 * be handled (run) by shell and put to user accessible directory.
 *
 *
**/

package .phantom;

//import .phantom.util.map;
// import .internal.bitmap;

/**
 *
 * Application: user-accessible object such as document, movie, etc.
 * Sumething that can be run from command line or double-clicked on in
 * graphics shell.
 *
**/

class application
{
    // TODO! Constructor!
    //.phantom.util.map env;


    void run( var command_line: .internal.string )
    {
    }

	// tell something about self
    .internal.string getDescription( )
    {
        return "(no description)";
    }

	// So that shell can paint icon of this object.
// .internal.bitmap getIcon();

	// TODO make it possible for object to update image and/or description dynamically

};



