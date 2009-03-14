//  Pair.java -- an index pair struct within the Constant table of a class files

package ru.dz.jpc.classfile;

class Pair {			// index pair struct
    int i1;
    int i2;
    public String toString() { return "    c" + i1 + " c" + i2; }
}
