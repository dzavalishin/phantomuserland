//  Handler.java -- an exception handler entry from a class file

package ru.dz.jpc.classfile;

public class Handler {	// one exception handler
    public int start;		// start of range
    public int end;		// end of range
    public int jump;		// jump address
    public ClassRef type;	// exception class ref (null if catches all)

    Handler(int s, int e, int t, ClassRef c) {
	start = s;
	end = e;
	jump = t;
	type = c;
    }
}
