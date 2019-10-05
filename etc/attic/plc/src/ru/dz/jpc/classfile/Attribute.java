//  Attribute.java -- Attribute handling

package ru.dz.jpc.classfile;

import java.io.*;



public class
Attribute {		// field, method, or class attribute

    String name;
    byte[] data;

public String
toString ()
{
    return name + "[" + data.length + "bytes]";
}

//  new Attribute(stream, ctab) -- load attribute from class file

Attribute(DataInputStream d, Constant c[]) throws IOException
{
    name = (String)c[d.readUnsignedShort()].value;
    d.readFully(data = new byte[d.readInt()]);
}



//  find(a, s) -- find an attribute in an array

public static byte[] find(Attribute a[], String s)
{
    for (int i = 0; i < a.length; i++)
       if (a[i].name.equals(s))
	    return a[i].data;
    return null;
}



} // class Attribute
