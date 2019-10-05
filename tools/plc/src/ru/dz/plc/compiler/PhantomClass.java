package ru.dz.plc.compiler;

import ru.dz.phantom.file.pclass.PhantomTypeInfo;
import ru.dz.plc.util.NameUse;
import ru.dz.plc.util.PlcException;
import ru.dz.soot.SootMain;

import java.io.*;
import java.util.*;


/**
 * <p>Class container.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class PhantomClass 
{
	static final String INTERNAL_CLASS_PREFIX = ".internal.";

	private String name;

	//	private String version; // Current class version, must increment each compile

	private MethodTable mt;
	private FieldTable ft; 
	private FieldTable staticFieldsTable; 
	private ConstantPool constantPool = new ConstantPool();

	private LinkedList<PhantomClass> interfaces = new LinkedList<PhantomClass>();
	private String parent = ".internal.object";

	private PhantomClass parent_class = null;
	private boolean have_nonvoid_parent = false;

	private Set<PhantomClass> referencedClasses;

	public PhantomClass(String name) throws PlcException {
		this.name = name;
		mt = new MethodTable();
		ft = new FieldTable();
		staticFieldsTable = new FieldTable();

		boolean noBase = name.equals(".internal.object") || name.equals(".internal.void"); 

		if (!noBase) {
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
		if (o == null || o.getClass() != this.getClass())
			return false;
		PhantomClass _t = (PhantomClass) o;

		return name != null && _t.name != null && name.equals( _t.name );
	}

	public String getShortName() {
		int pos = name.lastIndexOf(".");
		return (pos < 0) ? name : name.substring(pos + 1);
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
	//public boolean addParent(String name, ParseState ps) throws PlcException {
	public void addParent(String name, ParseState ps) throws PlcException {
		//if (have_nonvoid_parent)return false;
		if (have_nonvoid_parent)
			throw new PlcException("addParent", "already have base class for "+getName());
		
		parent = name;
		have_nonvoid_parent = true;

		parent_class = ClassMap.get_map().get(name,false,ps);

		// Try to import it 
		if (parent_class == null)
			ClassMap.get_map().do_import(name);

		parent_class = ClassMap.get_map().get(name,false,ps);

		if (parent_class == null)
			throw new PlcException("addParent", "can't load base class "+name, getName());

		if (parent_class != null) {
			// start assigning ordinals from where base class stopped
			mt.ordinals.setBase( parent_class.mt.slots_needed() );
			ft.setBase( parent_class.ft.slots_needed() );
			//ft.nextord = parent_class.ft.slots_needed();
		}

		//return parent_class != null;
	}

	// ------------------------------------------------------------------------
	// Constant pool
	// ------------------------------------------------------------------------

	public int addStringConst(String constant)
	{
		return constantPool.add(constant);
	}

	// ------------------------------------------------------------------------
	// Methods
	// ------------------------------------------------------------------------

	//@Deprecated
	protected Method findMethod(Method his_m) {
		Method m = mt.get(his_m.getSignature());
		if (m != null) return m;
		if (!have_nonvoid_parent)return null;
		return parent_class.findMethod(his_m);
	}
	/*
	@Deprecated
	public Method findMethod(String name) {
		Method m = mt.get(name);
		if (m != null) return m;
		if (!have_nonvoid_parent)return null;
		return parent_class.findMethod(name);
	}
	 */
	public Method findMethod(MethodSignature signature) {
		Method m = mt.get(signature);
		//return null; // TODO why?
		if (m != null) return m;
		if (!have_nonvoid_parent)return null;
		return parent_class.findMethod(signature);
	}

	/** Get just out method, don't search up */
	public Method getMethod(int ordinal) {
		return mt.get(ordinal);
	}

	public Method getDefaultConstructor() 
	{		
		// TODO hack?
		if(isInternal())
			return mt.get(0);
		
		List<PhantomType> args = new LinkedList<PhantomType>(); // no args
		MethodSignature signature = new MethodSignature(Method.CONSTRUCTOR_M_NAME, args);
		Method m = mt.get(signature);
		
		// TODO hack
		//if( (m == null) && ".internal.object".equals(getName()) )
		//	m = mt.get(0); // Method 0 is object defalut constructor
		
		return m;
	}


	static boolean isSameArgs(Method m1, Method m2) {
		Iterator<ArgDefinition> i1 = m1.getArgIterator();
		Iterator<ArgDefinition> i2 = m2.getArgIterator();

		while (i1.hasNext() && i2.hasNext()) 
		{
			ArgDefinition ad1 = i1.next();
			ArgDefinition ad2 = i2.next();

			PhantomType at1 = ad1.getType();
			PhantomType at2 = ad2.getType();
			
			if(! at1.equalsEvenUnknown(at2) )
				return false;
		}

		if (i1.hasNext() || i2.hasNext())
			return false;

		return true;
	}

	static String dumpArgs(Method m) { return m.dumpArgs(); }

	public static boolean isSameReturnType(Method m, Method bm) 
	{
		return !m.getType().equalsEvenUnknown(bm.getType());
	}

	/**
	 * Check if method has correct arguments if there is method in base class with the same name
	 * @param m
	 * @throws PlcException
	 */
	protected void checkBaseForMethod(Method m) throws PlcException 
	{
		if(!have_nonvoid_parent) return;
		
		Method bm = parent_class.findMethod(m); 
		
		if(bm == null) return;

		if(!isSameArgs(m, bm))
		{
			String ma1 = dumpArgs(m);	
			String ma2 = dumpArgs(bm);

			throw new PlcException("Method definition", "incompatible args",
					this.name + "::" + m.getName() + " required ("+ma2+"), have ("+ma1+")");
		}

		if( isSameReturnType(m, bm) )
			throw new PlcException("Method definition", "incompatible return type",
					this.name + "::" + m.getName() + " required ("+bm.getType()+"), have ("+m.getType()+")");
		
		// Here we do it
		m.setOrdinal(bm.getOrdinal());
	}

	/*
	@Deprecated
	public Method addMethod(String name, PhantomType type, boolean constructor ) throws PlcException {
		Method m = mt.add(name, type, constructor );
		//check_base_for_method(m);
		return m;
	}
	 */
	public Method addMethod(Method m) throws PlcException {
		//SootMain.say("adding method "+m);
		if(isNameUsedAsField(name))
			throw new PlcException(getName(), "name is used as field", name);

		
		mt.add(m);
		//check_base_for_method(m);
		return m;
	}

	/* public void add_method( String name, PhantomType type, Node args, Node code )
     { mt.add(name, type, args, code); }

     public void add_method( String name, PhantomType type, Node args, Node code, int ordinal ) throws PlcException
     { mt.add(name, type, args, code, ordinal ); } */

	protected void check_methods() throws PlcException {
		mt.set_ordinals(); // TODO double call?
		Iterator<Method> i = mt.iterator();
		while (i.hasNext()) {
			Method m = i.next();
			checkBaseForMethod(m);
		}
	}



	// ------------------------------------------------------------------------
	// Static Fields
	// ------------------------------------------------------------------------

	/**
	 * Add a static field to a class.
	 * @param name Name of field.
	 * @param type Type of field.
	 * @throws PlcException
	 */
	public void addStaticField(String name, PhantomType type) throws PlcException {
		// TODO parent?
		staticFieldsTable.add(name, type);
	}

	public PhantomField findStaticField(String name) {
		// TODO parent?
		return staticFieldsTable.get(name);
		//PhantomField f = staticFieldsTable.get(name);
		//if (f != null)return f;
		//if (!have_nonvoid_parent)return null;
		//return parent_class.find_field(name);
	}

	// ------------------------------------------------------------------------
	// Fields
	// ------------------------------------------------------------------------

	public PhantomField find_field(String name) {
		PhantomField f = ft.get(name);
		if (f != null)return f;
		if (!have_nonvoid_parent)return null;
		// TODO parent field visibility?
		return parent_class.find_field(name);
	}

	protected void check_base_for_field(String name, PhantomType type) throws PlcException {
		if (!have_nonvoid_parent)return;

		// TODO parent field visibility?
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
	 * @return 
	 * @throws PlcException
	 */
	public PhantomField addField(String name, PhantomType type) throws PlcException {
		//SootMain.say("add var '"+name+"' type "+type);
		
		if(isNameUsedAsMethod(name))
			throw new PlcException(getName(), "name is used as method", name);
		
		check_base_for_field(name, type);
		return ft.add(name, type);
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
	// Check for name to be used
	// ------------------------------------------------------------------------

	public NameUse isNameUsed(String name)
	{
		return new NameUse(ft.get(name) != null, staticFieldsTable.get(name) != null, mt.has(name));
	}
	
	public boolean isNameUsedAsMethod(String name)
	{
		return isNameUsed(name).isUsedAsMethod();
	}
		
	public boolean isNameUsedAsField(String name)
	{
		NameUse u = isNameUsed(name);
		return u.isUsedAsField() || u.isUsedAsStaticField();
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
			//Method m = mt.get(im.getName());
			Method m = mt.get(im.getSignature());

			if( m == null )
				throw new PlcException("class "+this.name +" definition",
						"Method "+im.getName()+" is required by interface "+iface.name);

			if (!isSameArgs(m, im))
				throw new PlcException("Method "+m.getName()+" definition",
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

	//public void codegen(RandomAccessFile os, FileWriter lst, BufferedWriter llvmFile, BufferedWriter c_File, String version) throws IOException, PlcException
	public void codegen(CodeWriters cw) throws IOException, PlcException
	{
		cw.llvmFile.write("; class "+getName()+"\n\n");
		cw.llvmFile.write("%OPTR = type <{ i8 *, i8 * }>\n");

		cw.c_File.write("// class "+getName()+"\n\n"); // TODO class version
		cw.c_File.write("#include <phantom/jit/generated.h>\n\n");

		cw.javaFile.write("public abstract class "+getName().substring(1)+" {\n");

		CodeGeneratorState s = new CodeGeneratorState(this);

		//ft.generateGettersSetters(this);

		//mt.codegen(os, lst, llvmFile, c_File, s, version);
		//ft.codegen(os, lst, llvmFile, c_File, s, version);
		//constantPool.codegen(os, lst, llvmFile, c_File, s, version);
		mt.codegen(cw,s);
		ft.codegen(cw,s);
		constantPool.codegen(cw);

		cw.javaFile.write("}; // end class "+getName().substring(1)+"\n");
	}

	public void createDefaultConstructor(ParseState ps) throws PlcException
	{
		ps.set_class(this);
		mt.checkDefaultConstructor(ps);
		ps.set_class(null);		
	}

	public void preprocess(ParseState ps) throws PlcException
	{
		//ft.generateGettersSetters(this);
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

	public void set_ordinals() throws PlcException {		mt.set_ordinals();	}
	public void propagateVoid() throws PlcException  { mt.propagateVoid();	}


	public int getFieldSlotsNeeded() {
		return ft.slots_needed();
	}

	public int getMethodSlotsNeeded() throws PlcException {
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

	/** Get getter for given field name */
	public Method getGetter(String ident) {
		return mt.get(FieldTable.makeGetterSignature(ident));
	}

	/** Get setter for given field name */
	public Method getSetter(String ident) {
		return mt.get(ft.makeSetterSignature(ident));
	}

	public void generateGettersSetters(ParseState ps) throws PlcException {
		ft.generateGettersSetters(this,ps);		
		//SootMain.say("class "+this);		mt.dump();
	}

	public void listMethods() {
		SootMain.say("class "+this);		
		mt.dump();
	}

	public boolean hasParent() {
		return have_nonvoid_parent;
	}

	/**
	 * Used to load class from object file. Sets field ordinal too.
	 * 
	 * @param fName
	 * @param fType
	 * @param fOrdinal
	 * @throws PlcException 
	 */
	public void setField(String fName, PhantomType fType, int fOrdinal) throws PlcException {
		ft.setField( fName, fType, fOrdinal );		
	}

	public void setPoolConstant(int cOrdinal, PhantomType pt, byte[] buf) throws PlcException, IOException {
		constantPool.setConstant(cOrdinal, pt, buf);		
	}


	/**
	 * Is this class internal (native).
	 * @return True if is.
	 */
	public boolean isInternal() 
	{		
		return INTERNAL_CLASS_PREFIX.equals(name.substring(0, INTERNAL_CLASS_PREFIX.length()));
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


