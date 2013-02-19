/**
 * 
 */
package ru.dz.soot;

import java.io.File;
import java.util.LinkedList;
import java.util.List;
import java.util.logging.Logger;

import soot.Body;
import soot.PatchingChain;
import soot.Scene;
import soot.SootClass;
import soot.SootMethod;
import soot.SootMethodRef;
import soot.Unit;
import soot.UnitBox;
import soot.ValueBox;
import soot.jimple.ArrayRef;
import soot.jimple.FieldRef;
import soot.jimple.InvokeExpr;
import soot.jimple.JimpleToBafContext;
import soot.jimple.internal.AbstractStmt;
import soot.jimple.internal.JAssignStmt;
import soot.jimple.internal.JGotoStmt;
import soot.jimple.internal.JIdentityStmt;
import soot.jimple.internal.JInvokeStmt;
import soot.jimple.internal.JReturnVoidStmt;
import soot.tagkit.Tag;

/**
 * @author dz
 *
 */
public class SootMain {
	Logger log = Logger.getLogger(SootMain.class.getName());

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub

		//String cp = "bin;../bin;lib/rt_6.jar";
		String cp = 
				"bin"+
				File.pathSeparator+
				"../bin"+
				File.pathSeparator+
				"lib/rt_6.jar";
		//System.setProperty("soot.class.path", cp);
		
		Scene.v().setSootClassPath(cp);
		
		SootClass c = Scene.v().loadClassAndSupport("test.toPhantom.SootTestClass");
		if( c.isPhantom() )
		{
			die("Not loaded");
		}

		List<SootMethod> mlist = c.getMethods();
		
		for( SootMethod m : mlist )
		{
			SootMethodTranslator mt = new SootMethodTranslator(m);
			//doMethod(m);
			mt.process();
		}
		
	}

	private static void die(String string) {
		System.err.println(string);
		System.exit(33);
	}

	private static void say(String string) {
		System.err.println(string);
	}

}
