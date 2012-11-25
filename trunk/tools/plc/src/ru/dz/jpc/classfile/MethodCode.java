/**
 * Abstract class to support code for methods
 * @version $Id: MethodCode.java,v 1.7 1998/12/30 15:57:29 pab Exp $
 * @author Peter A. Bigot
 *
 * Roughly corresponds to a C generic method structure mt_generic, but may
 * have additional information required for code generation.  It is
 * expected that this will be extended by specific implementations: the
 * interpreter code generator, or the JIT code generator, etc.
 */

/* This is part of the classfile component of the Toba system */
package ru.dz.jpc.classfile;

public class MethodCode {
	/* Toba method source type: tell us where the method came from.
	 * I suppose one could probably dig this out from the class loader,
	 * if one wanted to.  The values here should match those in the
	 * runtime toba.h file for the TobaMethodInvokerType enumeration. */
	static final int TMIT_undefined = 0; /* Unknown method */
	static final int TMIT_native_code = 1; /* Native code invocation */
	static final int TMIT_interpreter = 2; /* Call interpreter to execute */
	static final int TMIT_uninstalled_jit = 3; /* Need to compile this */
	static final int TMIT_abstract = 4; /* Unreachable abstract method */
	static int TMIT_LAST_ENUM = 4; /* Last assigned TMIT value */

	protected int itype;        // Tag indicating source of this method's code
	protected long mtentry;     // Java representation of function entry point 
	protected long stbentry;    // Java representation of function resolver stub entry point
	protected boolean isResolved; // Has this been linked yet?
	public Method method;       // The method to which the code belongs

	public static boolean shouldTimeCodegen;
	static {
		shouldTimeCodegen = (null != System.getProperty ("jtoba_timeCodegen"));
	}

	public int compileTime = 0; // Time taken to convert JVM to assembly code
	public int resolveTime = 0; // Time taken to resolve references in assembly code

	/** Create a new MethodCode instance belonging to given method
	 * @param m the method to which the code belongs
	 */
	public
	MethodCode (Method m) {     // Who wants code?
		/* Start out with an undefined type, no function pointer, and
		 * mark it unlinked. */
		itype = TMIT_undefined;
		mtentry = 0;
		isResolved = false;
		method = m;
	}

	/** Set the method code type
	 * @param it an integer value from the TMIT_* suite
	 * @returns the parameter it
	 */
	public int
	setMethodType (int it)      // Method code source/type tag
	{
		itype = it;
		return itype;
	}
	/** Retrieve the method code type
	 * @returns the code tag for this MethodCode instance
    public int
    getMethodType ()
    {
        return itype;
    }

    /** Set the method code function entry point
	 * @param fp address of native code function, cast to long
	 * @returns the parameter fp
	 */
	public long
	setMethodEntry (long fp)    // Where to call for this code
	{
		mtentry = fp;
		return mtentry;
	}
	/** Retrieve the method code function final entry point (never the
	 * stub entry point).
	 * @returns the method code function entry point
	 */
	public long
	getMethodEntry ()
	{
		return mtentry;
	}

	/** Retrieve the current method entry point, which is the real entry
	 * point if it's been resolved, and the stub entry point if not.
	 * @returns address of code to (resolve and) execute the method. */
	public long
	getEntryPoint ()
	{
		return isResolved ? mtentry : stbentry;
	}

	/** Set the method code stub entry point
	 * @param fp address of native code function, cast to long
	 * @returns the parameter fp
	 */
	public long
	setStubEntry (long fp)    // Where to call for this code
	{
		stbentry = fp;
		return stbentry;
	}
	/** Retrieve the method code stub entry point
	 * @returns the method code stub entry point
	 */
	public long
	getStubEntry ()
	{
		return stbentry;
	}

	public boolean
	getIsResolved () {
		return isResolved;
	}

	/** Do any linking that's necessary to complete code generation.
	 * This should be overridden if a particular compilation type has a
	 * nontrivial resolution action. */
	public void
	resolveCode ()
	{
		if (isResolved) {
			/* Already done this. */
			return;
		}
		isResolved = true;
		return;
	}
}
