package ru.dz.plc.compiler;

import ru.dz.plc.util.PlcException;

import java.io.*;
import java.util.*;

/**
 * <p>Class container.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class PhantomClass {

	private String name;

//	private String version; // Current class version, must increment each compile
	
	private MethodTable mt; // TODO accessed directly, must be public
	private FieldTable ft; // TODO accessed directly, must be public

	private LinkedList<PhantomClass> interfaces = new LinkedList<PhantomClass>();
	private String parent = ".internal.object";

	private PhantomClass parent_class = null;
	private boolean have_nonvoid_parent = false;

	private Set<PhantomClass> referencedClasses;

	public PhantomClass(String name) throws PlcException {
		this.name = name;
		mt = new MethodTable();
		ft = new FieldTable();

		if (!name.equals(".internal.object")) {
			parent_class = ClassMap.get_map().get(parent,false,null);
			if (parent_class == null)
				throw new PlcException("PhantomClass constructor",
						"no .internal.object class found", name);
			mt.ordinals.setBase( parent_class.mt.slots_needed() );
			ft.setBase( parent_class.ft.slots_needed() );
		}

	}

	public String getName() {		return name;	}

	public String getParent() {  return parent;  }

	public boolean equals(Object o) {
		if (o == null || o.getClass() != this.getClass())return false;
		PhantomClass _t = (PhantomClass) o;

		return name != null && _t.name != null && name == _t.name;
	}

	// ------------------------------------------------------------------------
	// Parents
	// ------------------------------------------------------------------------

	/**
	 * @param name Interface name
	 * @param ps Parse state to notify about dependency on this interface, or <code>null</code>.
	 */
	public void addInterface(String name, ParseState ps) {
		PhantomClass iface = ClassMap.get_map().get(name,false,ps);
		interfaces.add(iface);
	}

	/**
	 * @param name Parent name
	 * @param ps Parse state to notify about dependency on this parent, or <code>null</code>.
	 */
	public boolean addParent(String name, ParseState ps) throws PlcException {
		if (have_nonvoid_parent)return false;
		parent = name;
		have_nonvoid_parent = true;

		parent_class = ClassMap.get_map().get(name,false,ps);

		if (parent_class != null) {
			/*
      if (mt.nextord > 0 || ft.nextord > 0)
        throw new PlcException("parent class setup", "internal compiler error",
                         "mt.nextord > 0 || ft.nextord > 0");*/

			// start assigning ordinals from where base class stopped
			mt.ordinals.setBase( parent_class.mt.slots_needed() );
			ft.setBase( parent_class.ft.slots_needed() );
			//ft.nextord = parent_class.ft.slots_needed();
		}

		return parent_class != null;
	}

	// ------------------------------------------------------------------------
	// Methods
	// ------------------------------------------------------------------------

	protected Method findMethod(Method his_m) {
		Method m = mt.get(his_m.name);
		if (m != null) return m;
		if (!have_nonvoid_parent)return null;
		return parent_class.findMethod(his_m);
	}

	public Method findMethod(String name) {
		Method m = mt.get(name);
		if (m != null) return m;
		if (!have_nonvoid_parent)return null;
		return parent_class.findMethod(name);
	}
	
	static boolean isSameArgs(Method m1, Method m2) {
		Iterator<ArgDefinition> i1 = m1.args_def.iterator();
		Iterator<ArgDefinition> i2 = m2.args_def.iterator();

		while (i1.hasNext() && i2.hasNext()) {
			ArgDefinition ad1 = i1.next();
			ArgDefinition ad2 = i2.next();

			if (! (ad1.getType().equals(ad2.getType())))
				return false;
		}

		if (i1.hasNext() || i2.hasNext())
			return false;

		return true;
	}

	protected void check_base_for_method(Method m) throws PlcException {
		if (!have_nonvoid_parent)return;
		Method bm = parent_class.findMethod(m);
		if (bm == null)return;

		if (!isSameArgs(m, bm))
			throw new PlcException("Method definition", "incompatible args",
					this.name + "::" + m.name);

		// Here we do it
		m.ordinal = bm.ordinal;
	}

	public Method addMethod(String name, PhantomType type) throws PlcException {
		Method m = mt.add(name, type);
		//check_base_for_method(m);
		return m;
	}

	public Method addMethod(Method m) throws PlcException {
		mt.add(m);
		//check_base_for_method(m);
		return m;
	}

	/* public void add_method( String name, PhantomType type, Node args, Node code )
     { mt.add(name, type, args, code); }

     public void add_method( String name, PhantomType type, Node args, Node code, int ordinal ) throws PlcException
     { mt.add(name, type, args, code, ordinal ); } */

	protected void check_methods() throws PlcException {
		Iterator<Method> i = mt.iterator();
		while (i.hasNext()) {
			Method m = i.next();
			check_base_for_method(m);
		}
	}

	// ------------------------------------------------------------------------
	// Fields
	// ------------------------------------------------------------------------

	public PhantomField find_field(String name) {
		PhantomField f = ft.get(name);
		if (f != null)return f;
		if (!have_nonvoid_parent)return null;
		return parent_class.find_field(name);
	}

	protected void check_base_for_field(String name, PhantomType type) throws PlcException {
		if (!have_nonvoid_parent)return;

		//PhantomField f = ft.get(name);
		PhantomField f = parent_class.find_field(name);
		if (f == null)return;

		throw new PlcException("field definition",
				"there is a field with this name in a base class",
				this.name + "::" + name);

		//if(f.get_type() != type )      throw
	}

	/**
	 * Add a field to a class.
	 * @param name Name of field.
	 * @param type Type of field.
	 * @throws PlcException
	 */
	public void addField(String name, PhantomType type) throws PlcException {
		check_base_for_field(name, type);
		ft.add(name, type);
	}

	/** 
	 * used by java class importer 
	 * @throws PlcException 
	 */
	public void setField(int phantomOrdinal, String varName, PhantomType type) throws PlcException {
		check_base_for_field(name, type);
		ft.set(phantomOrdinal, name, type);
	}
	
	
	// ------------------------------------------------------------------------
	// Interface compliance
	// ------------------------------------------------------------------------

	protected void check_interface_methods_compliance(PhantomClass iface) throws
	PlcException {
		Iterator<Method> i = iface.mt.iterator();
		while( i.hasNext() )
		{
			Method im = i.next();

			// find corresponding my Method
			Method m = mt.get(im.name);

			if( m == null )
				throw new PlcException("class "+this.name +" definition",
						"Method "+im.name+" is required by interface "+iface.name);

			if (!isSameArgs(m, im))
				throw new PlcException("Method "+m.name+" definition",
						"args are incompatible with interface "+iface.name);

		}
	}


	protected void check_interface_compliance(PhantomClass curr_iface) throws PlcException {
		Iterator<PhantomClass> i = curr_iface.interfaces.iterator();
		while( i.hasNext() )
		{
			PhantomClass iface = i.next();
			check_interface_methods_compliance(iface);
			check_interface_compliance(iface);
		}
	}


	protected void check_interfaces_compliance() throws PlcException {
		check_interface_compliance(this);
	}

	// ------------------------------------------------------------------------
	// General
	// ------------------------------------------------------------------------

	public String toString() { return name; }

	public void print(PrintStream ps) throws PlcException
	{
		
		ps.println("Class "+name);
		//System.out.println("  Fields:");
		ft.print(ps);
		//System.out.println("  Methods:");
		mt.print(ps);
	}

	public void codegen(RandomAccessFile os, FileWriter lst, String version) throws IOException, PlcException
	{
		CodeGeneratorState s = new CodeGeneratorState(this);
		mt.codegen(os, lst, s, version);
		ft.codegen(os, lst, s, version);
	}

	public void preprocess(ParseState ps) throws PlcException
	{
		check_methods();
		check_interfaces_compliance();
		ps.set_class(this);
		mt.preprocess(ps);
		ps.set_class(null);
	}

	public boolean can_be_assigned_from( PhantomClass src )
	{
		if( src == null  )
		{
			System.out.println("Null src class in can_be_assigned_from");
			return false;
		}

		if( name.equals(src.name) ) return true;

		if( src.parent_class != null )
			return can_be_assigned_from( src.parent_class );

		return false;
	}

	public void set_ordinals() {
		mt.set_ordinals();		
	}

	public int getFieldSlotsNeeded() {
		return ft.slots_needed();
	}

	public int getMethodSlotsNeeded() {
		return mt.slots_needed();
	}

	public void setMethodOrdinal(Method m, int required_method_index) throws PlcException {
		mt.set_ordinal(m,required_method_index);		
	}

	public void setReferencedClasses(Set<PhantomClass> referencedClasses) {
		this.referencedClasses = referencedClasses;
	}

	public Set<PhantomClass> getReferencedClasses() {
		return referencedClasses;
	}



}


class ordinals_generator
{
	private int       nextord = 0;
	private boolean   accessed = false; // at least one ordinal was given out

	void setBase(int base) throws PlcException {
		if( accessed )
			throw new PlcException("ordinals_generator", "internal compiler error", "tried to set base after ordinal was given out");
		nextord = base;
	}

	public void ensureBase(int base) throws PlcException {
		if( accessed )
			throw new PlcException("ordinals_generator", "internal compiler error", "tried to set base after ordinal was given out");
		if(base > nextord)
			nextord = base;
	}
	
	int getNext()
	{
		accessed = true;
		return nextord++;
	}


}


