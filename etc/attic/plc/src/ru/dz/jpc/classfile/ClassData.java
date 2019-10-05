//  ClassData.java -- the data from a Java class file

package ru.dz.jpc.classfile;

import java.io.*;
import java.util.*;

public class ClassData {

	/**
	 * We need to keep a mapping from (name,ClassLoader) pairs to ClassData
	 * structures, so we can get back a consistent ClassData structure when we
	 * lookup interfaces by name. We may not have the classfile available to
	 * re-read. Note that the fact that we can't use null as a hash key means we
	 * need to swap in something else to represent the system class loader.
	 */
	private static Hashtable name2cdata;
	private static Object pseudoSystemClassLoader = new Object();

	/**
	 * Given the name of a class, and the loader that's supposed to define it,
	 * dig out the ClassData structure we associated with that pair; or return
	 * null if there is none.
	 */
	private static ClassData getByNameLoader(String nm, ClassLoader cld) {
		Hashtable cdh;
		Object ocld;

		/* Key is the class loader, or its valid substitute */
		ocld = cld;
		if (null == ocld) {
			ocld = pseudoSystemClassLoader;
		}
		/* Get the hash from loaders to data structures. */
		cdh = (Hashtable) name2cdata.get(nm);
		if (null != cdh) {
			return (ClassData) cdh.get(ocld);
		}
		return null;
	}

	/**
	 * Record a ClassData structure so if we want to find a named class relative
	 * to a given class loader, we don't have to try to reconstruct the raw
	 * class information.
	 */
	private static void addByNameLoader(String nm, ClassLoader cld, ClassData cd) {
		Hashtable cdh;
		Object ocld;

		ocld = cld;
		if (null == ocld) {
			ocld = pseudoSystemClassLoader;
		}

		cdh = (Hashtable) name2cdata.get(nm);
		if (null == cdh) {
			cdh = new Hashtable();
			name2cdata.put(nm, cdh);
		}
		cdh.put(ocld, cd);
		return;
	}

	static private Runtime rts;
	static {
		name2cdata = new Hashtable();
		rts = Runtime.getRuntime();
	}

	// instance variables

	/** class name */
	public String name; 
	/** name as used in C */
	public String cname; 
	public String fname; // name used for toba file
	/** superclass name */
	public String supername; 
	public int major, minor; // version numbers
	public int access; // access flags and other flags
	public Constant[] constants; // constant table
	public ClassRef[] interfaces; // interfaces implemented
	public Field[] fields; // fields
	public Field[] methods; // methods
	public Hashtable symtab; // symbol table (fields & methods)
	public Attribute[] attributes; // class attributes

	// built after superclass has been loaded by client
	/** instance method table */
	public Field[] imtable; 
	/** static method table */
	public Field[] smtable; 
	/** instance variable table */
	public Field[] ivtable; 
	/** class variable table */
	public Field[] cvtable; 

	public Hashtable visiblevars; // Hashtable of visible instance
	// fields

	// Not set by constructor Client must set.
	// (e.g. toba.translator.Super)
	public ClassData superclass; // superclass data

	// Not set by constructor Client must set.
	// (e.g. toba.runtime.CodeGen)
	public Class javaClass; // The java runtime counterpart
	// of this class

	public ClassRef myRef; // A ClassRef for this class

	/** records what resolution has been done to this class */
	public int state; 

	public int cdsrc; // Where did this come from?

	// potential states for ClassData
	public static final int RES_NONE = 0;
	public static final int RES_SUPERCLASSES = 1;
	public static final int RES_CONSTANTS = 2;

	// flags from Java class files
	public static final int ACC_PUBLIC = 0x0001;
	public static final int ACC_PRIVATE = 0x0002;
	public static final int ACC_PROTECTED = 0x0004;
	public static final int ACC_STATIC = 0x0008;
	public static final int ACC_FINAL = 0x0010;
	public static final int ACC_SYNCHRONIZED = 0x0020;
	public static final int ACC_VOLATILE = 0x0040;
	public static final int ACC_TRANSIENT = 0x0080;
	public static final int ACC_NATIVE = 0x0100;
	public static final int ACC_INTERFACE = 0x0200;
	public static final int ACC_ABSTRACT = 0x0400;

	// Source of the classdata structure
	public static final int CDSRC_classfile = 0; // Some JVM class structure
	public static final int CDSRC_tobaclass = 1; // A pre-compiled Toba class
													// struct

	// set by IHash.mark() ONLY IN TRANSLATOR (?)

	public static final int FLG_INTERFACE = 0x8000; // interface method

	// new ClassData(d) -- load class data from DataInputStream d
	//
	// Note: the file is closed after loading.

	/**
	 * Initialize a basic ClassData structure from the data in the toba Class
	 * structure at claddr.
	 * 
	 * @param cld
	 *            raw ClassData instance to be filled in
	 * @param cl
	 *            Java Class which gets us access to native Class structure.
	 *            Toba hash code: _CC_a8RIy
	 */
	private static native void ClassDataInternal(ClassData cld, Class cl);

	/**
	 * Add an array of fields to the symbol table.
	 * 
	 * @param farr
	 *            fields to be added
	 */
	private void addSymFields(Field farr[]) {
		int i;
		Field f;

		for (i = 0; i < farr.length; i++) {
			f = farr[i];
			f.next = (Field) symtab.get(f.name);
			symtab.put(f.name, f);
		}
		return;
	}

	/**
	 * Constructors are private: data should be located by name or file, to
	 * ensure uniqueness. This one reconstructs the data from an existing Class
	 * object.
	 */
	private ClassData(Class cl) {
		int i;

		// Use the native function to fill in what we can from the C structure
		ClassDataInternal(this, cl);

		// Complete filling out the stuff that's easier to do up here.
		/* Basic name mangling */
		cname = Names.hashclass(name);
		fname = Names.classfile(name);
		major = 45;
		minor = 3;

		/*
		 * Mark this as from a Toba class, so we don't try to resolve things
		 * that have already been set up.
		 */
		cdsrc = ClassData.CDSRC_tobaclass;

		/* Put fields and methods into symbol table */
		symtab = new Hashtable();
		for (i = 0; i < fields.length; i++) {
			Field f = fields[i];
			f.cname = Names.hashvar(f.name);
		}
		addSymFields(fields);
		for (i = 0; i < methods.length; i++) {
			Field f = methods[i];
			f.cname = Names.hashmethod(f.name, this.name, f.signature);
		}
		addSymFields(methods);

		/* Hope we don't need any attributes, cuz they're long gone. */
		attributes = new Attribute[0];

		/* We haven't done any resolution yet. */
		state = RES_NONE;

		return;
	}

	/* This one sets up ClassData based on data in a class file. */
	private ClassData(DataInputStream d) throws ClassFormatError, IOException {
		symtab = new Hashtable();

		// read header
		if (d.readInt() != 0xCafeBabe)
			throw new ClassFormatError("bad magic number");
		minor = d.readUnsignedShort();
		major = d.readUnsignedShort();

		// read constant table
		constants = Constant.load(d);

		// read access flags, class, superclass
		access = d.readUnsignedShort();
		name = ((ClassRef) constants[d.readUnsignedShort()].value).name;
		cname = Names.hashclass(name);
		fname = Names.classfile(name);
		int i = d.readUnsignedShort();
		if (i > 0)
			supername = ((ClassRef) constants[i].value).name;

		// read interface list
		interfaces = new ClassRef[d.readUnsignedShort()];
		for (i = 0; i < interfaces.length; i++) {
			int j = d.readUnsignedShort();
			if (j > 0)
				interfaces[i] = (ClassRef) constants[j].value;
			else
				interfaces[i] = null;
		}

		// read fields
		fields = new Field[d.readUnsignedShort()];
		for (i = 0; i < fields.length; i++) {
			Field f = new Field(d, constants);
			// for naming purposes, assume no shadowing
			f.cname = Names.hashvar(f.name);
			f.classfilePos = i;
			fields[i] = f;
		}
		addSymFields(fields);

		// read methods
		methods = new Field[d.readUnsignedShort()];
		for (i = 0; i < methods.length; i++) {
			Field f = new Field(d, constants);
			f.cname = Names.hashmethod(f.name, this.name, f.signature);
			f.classfilePos = i;
			methods[i] = f;
		}
		addSymFields(methods);

		// read class attributes

		attributes = new Attribute[d.readUnsignedShort()];
		for (i = 0; i < attributes.length; i++)
			attributes[i] = new Attribute(d, constants);

		// no resolution has been done on this class
		this.myRef = ClassRef.byName(name);
		state = RES_NONE;

		// all done
		d.close(); // close file for convenience of caller
		cdsrc = ClassData.CDSRC_classfile;

		return;
	}

	public static ClassData forStream(ClassLoader cld, DataInputStream d)
			throws IOException {
		return ClassData.forStream(cld, d, true);
	}

	public static ClassData forStream(ClassLoader cld, DataInputStream d,
			boolean rememberp) throws IOException {
		ClassData ncd;
		ClassData ocd;

		/* Create a ClassData, so we know what name we're looking for. */
		ncd = new ClassData(d);

		/*
		 * If we already have a ClassData structure for this name, return that
		 * instead.
		 */
		ocd = getByNameLoader(ncd.name, cld);
		if (null != ocd) {
			ncd = ocd;
		} else {
			/* Didn't have one before; memorize this one, if asked to do so. */
			if (rememberp) {
				addByNameLoader(ncd.name, cld, ncd);
			}
		}

		/* Return what we got. */
		return ncd;
	}

	public static ClassData forStream(DataInputStream d) throws IOException {
		ClassData ncd;

		/* Create a ClassData, so we know what name we're looking for. */
		ncd = new ClassData(d);

		/* Return what we got. */
		return ncd;
	}
	
	
	/**
	 * Get a ClassData structure corresponding to the given Class, which must
	 * have been resolved.
	 */
	public static ClassData forClass(Class cl) {
		ClassData cd;

		/* If it's in the hash table, we're OK */
		cd = getByNameLoader(cl.getName(), cl.getClassLoader());

		/* If it's not in the hash table, we're supposed to make one. Do so. */
		if (null == cd) {
			/*
			 * Two caveats here: this only works when we're running in a Toba
			 * runtime system, because we'll be using a native function to parse
			 * the internal class structure. Second, we _must_ remember this
			 * thing, because we're storing a pointer to it in the internal
			 * structure (the pre-compiled struct class), and that's not scanned
			 * for GC
			 */
			cd = new ClassData(cl);
			addByNameLoader(cl.getName(), cl.getClassLoader(), cd);
		}

		/* Return what we got. */
		return cd;
	}

	/** Get a ClassData structure corresponding to the given name. */
	public static ClassData forName(String nm) throws ClassNotFoundException {
		ClassData cd;

		/*
		 * This is only to be called within the translator, i.e. when there's no
		 * confusion about class loaders
		 */

		/* If it's in the hash table, we're OK */
		cd = getByNameLoader(nm, null);

		/* If it's not in the hash table, we're supposed to make one. Do so. */
		if (null == cd) {
			cd = ClassData.forClass(Class.forName(nm));
		}

		/* Return what we got. */
		return cd;
	}

	/**
	 *  getmethod(name, climb) -- find "()V" method by name.
	 * @param name
	 * @param climb If climb is true, and if superclasses have been loaded, check them too.
	 * @return
	 */
	public Field getmethod(String name, boolean climb) {
		for (ClassData k = this; k != null; k = k.superclass) {
			for (Field m = (Field) k.symtab.get(name); m != null; m = m.next) {
				if (m.signature.equals("()V")) {
					return m;
				}
			}
			if (!climb)
				break;
		}
		return null;
	}

	// Methods for dealing with superclasses, once they have been set.

	// buildMethodTables(class) -- scan methods and build method tables.
	//
	// Assumes that superclasses have been loaded.
	private void buildMethodTables(boolean setSlots) {
		int mtsize;
		int stsize = 0;

		if (imtable != null)
			return; // method table already initialized

		if (superclass == null) {
			mtsize = 0;
		} else {
			mtsize = superclass.imtable.length;
		}

		// assign method table slots, checking for overrides of ancestors.
		nextmethod: for (int i = 0; i < methods.length; i++) {
			Field f = methods[i];
			if ((f.access & ClassData.ACC_STATIC) != 0) {
				/* Give static method a slot in the static table for this class */
				if (setSlots) {
					f.tableslot = stsize;
				} else {
					if (f.tableslot != stsize) {
						throw new InternalError(
								"C class static method slot mismatch");
					}
				}
				stsize++;
			} else {

				// this is a virtual method
				String name = f.name;
				int gen = 0;
				for (ClassData c = superclass; c != null; c = c.superclass) {
					gen++;
					for (Field a = (Field) c.symtab.get(name); a != null; a = a.next) {
						if ((a.access & ClassData.ACC_PRIVATE) != 0)
							continue;
						if (f.signature.equals(a.signature)) {
							// this method overrides an ancestor
							f.overrides = a.overrides + gen;
							if (setSlots) {
								f.tableslot = a.tableslot;
							} else {
								if (f.tableslot != a.tableslot) {
									throw new InternalError(
											"C class method instance override slot mismatch");
								}
							}
							continue nextmethod;
						}
					}
				}

				// method does not override an ancestor, so it
				// gets a method table slot

				if (setSlots) {
					f.tableslot = mtsize;
				} else {
					if (f.tableslot != mtsize) {
						throw new InternalError(
								"C class method instance slot mismatch");
					}
				}
				mtsize++;
			}
		}

		// allocate and initialize instance method table
		imtable = new Field[mtsize];
		smtable = new Field[stsize];
		if (superclass != null)
			System.arraycopy(superclass.imtable, 0, imtable, 0,
					superclass.imtable.length);
		for (int i = 0; i < methods.length; i++) {
			Field f = methods[i];
			if ((f.access & ClassData.ACC_STATIC) == 0)
				imtable[f.tableslot] = f;
			else
				smtable[f.tableslot] = f;
		}

	}

	// ivregister(c, h) -- register instance variables in visibility table
	//
	// Put our visible fields into the visibility hash table. Assumes that
	// the table alreay contains the visible fields for out superclass
	private void ivregister() {
		for (int i = 0; i < fields.length; i++) {
			Field f = fields[i];
			if ((f.access & ClassData.ACC_STATIC) == 0)
				visiblevars.put(f.name, f);
		}
	}

	// buildVariableTables(k) - Build table of instance and static variables.
	// Compute a hash table of
	// visible instance fields, also.
	private void buildVariableTables(boolean setSlots) {
		int ivtsize;
		int svtsize = 0; // static variables are not inherited.

		if (!setSlots) {
			// System.out.println ("buildVariableTables on tobaclass:\n" +
			// this);
		}
		/*
		 * Static vars aren't inherited, so start with zero; instance ones are,
		 * so start with superclass values if present.
		 */
		if (superclass != null) {
			visiblevars = (Hashtable) superclass.visiblevars.clone();
			ivtsize = superclass.ivtable.length;
		} else {
			visiblevars = new Hashtable();
			ivtsize = 0;
		}

		ivregister();

		// Compute the number of static and instance variables,
		// and record their offset in the table as we go.
		for (int i = 0; i < fields.length; i++) {
			Field f = fields[i];
			// System.out.println ("Field " + f.name + " slot " + f.tableslot +
			// "; ivt=" +
			// ivtsize + ", svt=" + svtsize);
			if ((f.access & ClassData.ACC_STATIC) == 0) {
				if (setSlots) {
					f.tableslot = ivtsize;
				} else {
					if (f.tableslot != ivtsize) {
						throw new InternalError(
								"C class instance var slot mismatch: " + name
										+ ", iv " + f.name + ", asg "
										+ f.tableslot + " and inferred "
										+ ivtsize);
					}
				}
				++ivtsize;
			} else {
				if (setSlots) {
					f.tableslot = svtsize;
				} else {
					if (f.tableslot != svtsize) {
						throw new InternalError(
								"C class static var slot mismatch");
					}
				}
				++svtsize;
			}
		}

		ivtable = new Field[ivtsize];
		cvtable = new Field[svtsize];

		// Put all of our new variables into appropriate tables. We copy the
		// table of instance variables from our superclass.

		// copy the appropriate information from our superclass
		if (superclass != null)
			System.arraycopy(superclass.ivtable, 0, ivtable, 0,
					superclass.ivtable.length);

		for (int i = 0; i < fields.length; i++) {
			Field f = fields[i];
			if ((f.access & ClassData.ACC_STATIC) == 0) {
				ivtable[f.tableslot] = f;
			} else {
				cvtable[f.tableslot] = f;
			}
		}
	}

	// buildTables(k) - build the method and variable tables for this class.
	// assumes that these tables have already been built for my superclass
	public void buildTables() {
		/*
		 * Do not screw with the table slots if this is a toba-generated
		 * whatsis. They're already right.
		 */
		buildMethodTables(CDSRC_tobaclass != cdsrc);
		buildVariableTables(CDSRC_tobaclass != cdsrc);
	}

	private String arrayString(Object ob[]) {
		if (null == ob) {
			return "null";
		} else {
			StringBuffer str = new StringBuffer(Integer.toString(ob.length)
					+ ": ");
			int i;

			for (i = 0; i < ob.length; i++) {
				str.append(ob[i] + " ");
			}
			return str.toString();
		}
	}

	public String toString() {
		StringBuffer str = new StringBuffer("");

		str.append("name: " + name + "\n");
		str.append("cname: " + cname + "\n");
		str.append("fname: " + fname + "\n");
		str.append("supername: " + supername + "\n");
		str.append("(major, minor): " + major + ", " + minor + "\n");
		str.append("access: 0x" + Integer.toHexString(access) + "\n");
		str.append("Constants: " + arrayString(constants) + "\n");
		str.append("Interfaces: " + arrayString(interfaces) + "\n");
		str.append("Fields: " + arrayString(fields) + "\n");
		str.append("Methods: " + arrayString(methods) + "\n");
		str.append("Inst Methods: " + arrayString(imtable) + "\n");
		str.append("Stat Methods: " + arrayString(smtable) + "\n");
		str.append("Attributes: " + arrayString(attributes) + "\n");
		str.append("Source: " + cdsrc + "\n");
		str.append("State: " + state + "\n");
		return str.toString();
	}

} // class ClassData
