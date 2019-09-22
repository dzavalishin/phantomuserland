package ru.dz.plc.compiler;

import java.util.HashSet;
import java.util.Set;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: </p>
 * @author dz
 * @version 1.0
 */

public class ParseState
extends GeneralState 
{
	public ParseState(PhantomClass c)
	{
		super(c);
	}
	
	public ParseState()
	{		
	}

	public ParseState(ParseState ps) 
	{
		super(ps);
		referencedClasses = new HashSet<PhantomClass>(ps.referencedClasses);
	}
	
	public void set_class ( PhantomClass c ) { my_class = c; }


	private Set<PhantomClass> referencedClasses = new HashSet<PhantomClass>();  
	
	
	public void addReferencedClass(PhantomClass c)
	{
		referencedClasses.add(c);
	}

	public Set<PhantomClass> getReferencedClasses() {
		return referencedClasses;
	}


}

