//  Repr.java -- methods dealing with C representations

package ru.dz.jpc.tanslator;

import ru.dz.jpc.classfile.*;
import java.io.*;

@Deprecated
class Repr {

static final int DECLS_PER_LINE = 15;	// variables per declaration line
static final int GCHARS_PER_LINE = 18;	// generated char consts on one line
static final int GCHARS_PER_CMMT = 40;	// max chars of string gen in comment


//  ctype(c) -- return C type indicated by character c.
//
//  upper case characters are Java signature characters.
//  lower case characters are JVM datatypes.

static String ctype(char c)
{
    switch (c) {
	case 'B':             return "Byte";
	case 'C':             return "Char";
	case 'D':  case 'd':  return "Double";
	case 'F':  case 'f':  return "Float";
	case 'I':  case 'i':  return "Int";
	case 'J':  case 'l':  return "Long";
	case 'S':             return "Short";
	case 'V':             return "Void";
	case 'Z':             return "Boolean";
	default:              return "Object";
    }
}



//  rettype(s) -- Compute C return type of Java method given signature.

static String rettype(String s)
{
    return ctype(s.charAt(s.indexOf(')') + 1));
}




//  isQuotable(c) -- is character c reasonably specified as 'c'?

static boolean isQuotable(char c)
{
    return c >= ' ' && c <= '~' && c != '\'' && c != '\\';
}



//  con(m, c) -- return C representation of constant c in method m.
//
//  Valid for INT, LONG, FLOAT, DOUBLE, or STRING constants
//  (assuming that necessary static constants were generated earlier).

static String con(Method m, Constant c)
{
    switch (c.tag) {

	case Constant.INTEGER:
	    if (((Integer)c.value).intValue() == Integer.MIN_VALUE)
		return "0x80000000";
	    else
		return c.value.toString();

	case Constant.LONG:
	    long v = ((Long)c.value).longValue();
	    if (v > (long)Integer.MIN_VALUE && v < (long)Integer.MAX_VALUE)
		return c.value.toString();
	    // now we must use "ANSI C" to construct a possibly "long long" val
	    int lh = (int)(v >>> 32);
	    int rh = (int)v;
	    return "((((Long)0x" + Integer.toHexString(lh) +
		"u) << 32) | ((Long)0x" + Integer.toHexString(rh) + "u))";

	case Constant.FLOAT:
	case Constant.DOUBLE:
	    if (simplefloat(((Number)c.value).doubleValue()))
		return c.value.toString();
	    else if (c.tag == Constant.FLOAT)
		return "fc" + c.index + ".v";
	    else /* c.tag == Constant.DOUBLE */
		return "dc" + c.index + ".v";

	case Constant.STRING:
	    return "(Object)st_" + m.cl.cname + "[" + CFile.strref(c) + "]";

	default:
	    return c.value.toString();
    }
}



//  simplefloat(d) -- is d represented accurately by toString()?
//
//  This method errs on the side of caution, but is good enough
//  to accept many exact numbers such as 23.0, 1.5, and 0.125.

static boolean simplefloat(double d)
{
    if (Double.isNaN(d) || Double.isInfinite(d))
	return false;
    if (Double.doubleToLongBits(d) == 0x8000000000000000L)
	return false;			// negative zero

    if (d % 1.0 == 0.0 && d < 1000000.0 && d > -1000000.0)
	return true;
    if (d % 0.03125 == 0.0 && d < 10.0 && d > -10.0)
	return true;

    return false;
}

/** Write to stream a sequence of comma-separated integral expressions which
  * can be used to initialize an array of unsigned shorts representing a
  * Unicode string. */
static public void
emitCharData (PrintWriter d,    // Where to write data
              String strdata)   // String containing data
{
    int slen = strdata.length ();
    int i = 0;
    while (i < slen) {
        char c = strdata.charAt (i);
        if (isQuotable (c)) {
            /* Consider this a printable that can appear as a C char constant */
            d.print("'" + c + "'");
        } else {
            /* A non-printable; treat as an integer */
            d.print((int) c);
        }
        if (++i < slen) {
            /* Separate with commas */
            d.print (",");
            if (0 == (i % GCHARS_PER_LINE)) {
                /* Keep lines from getting too big */
                d.print ("\n");
            }
        }
    }
    return;
}

} // class Repr
