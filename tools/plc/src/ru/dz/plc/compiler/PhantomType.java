package ru.dz.plc.compiler;

import ru.dz.phantom.code.*;
import ru.dz.phantom.file.pclass.PhantomTypeInfo;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.*;

import java.io.*;

/**
 * <p>Phantom VM Type. Class + possible container attributes.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2016 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class PhantomType 
{
	public static final String DEFAULT_CONTAINER_CLASS = ".internal.container.array";
	
	protected PhantomClass        _class;
	protected PhantomClass        _container_class;
	protected boolean             _is_void;
	protected boolean             _is_container;
	protected boolean             _is_known;

	protected boolean             _is_int;
	protected boolean             _is_long;
	protected boolean             _is_float;
	protected boolean             _is_double;

	protected boolean             _is_string;

	protected Node                _class_expression;
	protected Node                _container_class_expression;

	protected boolean 			  iAmStatic;

	public PhantomType(PhantomType src) 
	{
		this._class = src._class;
		this._class_expression = src._class_expression;
		this._container_class = src._container_class;
		this._container_class_expression = src._container_class_expression;
		
		this._is_container = src._is_container;
		
		this._is_double = src._is_double;
		this._is_float = src._is_float;
		this._is_int = src._is_int;
		this._is_long = src._is_long;

		this._is_string = src._is_string;
		this._is_void = src._is_void;
		
		this.iAmStatic = src.iAmStatic;
		
		this._is_known = src._is_known;		
	}
	
	
	public String get_main_class_name()
	{
		if(_is_container)  return _container_class == null ? DEFAULT_CONTAINER_CLASS : _container_class.getName();
		if(_is_void)       return ".internal.void";
		if(_is_int)        return ".internal.int";
		if(_is_long)       return ".internal.long";
		if(_is_float)      return ".internal.float";
		if(_is_double)     return ".internal.double";
		if(_is_string)     return ".internal.string";
		return _class == null ? "" : _class.getName();
	}

	public String get_contained_class_name()
	{
		if(_is_container)
			return _class == null ? "" : _class.getName();
		return "";
	}

	protected PhantomType()
	{
		_is_void = false;
		_is_string = false;
		_is_int = false;
		_is_long = false;
		_is_float = false;
		_is_double = false;
		_is_container = false;
		_is_known = true;
		_class = null;
		_container_class = null;
	}

	public PhantomType(PhantomClass _c) //throws PlcException
	{
		//if( _c == null )			throw new PlcException("PhantomType(PhantomClass)","Class is null");

		_is_void = false;
		_is_string = false;
		_is_int = false;
		_is_long = false;
		_is_float = false;
		_is_double = false;
		_is_container = false;
		_is_known = (_c != null);
		_class = _c;
		_container_class = null;

		if( _c != null && _c.getName().equals(".internal.int") ) _is_int = true;
		if( _c != null && _c.getName().equals(".internal.long") ) _is_long = true;
		if( _c != null && _c.getName().equals(".internal.float") ) _is_float = true;
		if( _c != null && _c.getName().equals(".internal.double") ) _is_double = true;
		if( _c != null && _c.getName().equals(".internal.string") ) _is_string = true;
		if( _c != null && _c.getName().equals(".internal.void") ) _is_void = true;

		// TODO why not true? .object is void?
		//if( _c != null && _c.get_name().equals(".internal.object") ) _is_void = true;
	}

	public PhantomType(PhantomClass _c, boolean is_container )
	{
		_is_void = false;
		_is_string = false;
		_is_int = false;
		_is_long = false;
		_is_float = false;
		_is_double = false;
		_is_known = true;
		_is_container = is_container;
		_class = _c;
		_container_class = null;
	}

	public boolean is_void() { return _is_void; }
	public boolean is_int() { return _is_int; }
	public boolean is_long() { return _is_long; }
	public boolean is_float() { return _is_float; }
	public boolean is_double() { return _is_double; }
	public boolean is_string() { return _is_string; }
	public boolean is_container() { return _is_container; }
	public boolean is_unknown() { return !_is_known; }

	//public void set_is_container(boolean is_container) {		this._is_container = is_container;	}
	public PhantomType toContainer() 
	{
		PhantomType ret = new PhantomType(this);
		ret._is_container = true;
		return ret;
	}

	public boolean is_on_int_stack() { return _is_int||_is_long||_is_float||_is_double; }
	
	public boolean isNumeric() { return is_on_int_stack(); }

	/**
	 * Does it take 2 x 32 bit slots 
	 * @return True for 64 bit numeric types.
	 */
	public boolean is64bit() { return _is_long||_is_double; }
	
	
	/**
	 * NB! Any unknown type is assumed to be different from any other.
	 * 
	 * TODO does not conform to equals() contract! Containers will work strangely.
	 */
	public boolean equals( Object o )
	{
		if( o == null || ! (o instanceof PhantomType ) ) return false;

		PhantomType _t = (PhantomType)o;

		if( !_is_known || !_t._is_known ) return false; // TODO really? leave it for caller to check for special cases? Throw?

		if( _is_void && _t._is_void ) return true;
		if( _is_void || _t._is_void ) return false;


		if( _is_int && _t._is_int ) return true;
		if( _is_int || _t._is_int ) return false;

		if( _is_long && _t._is_long ) return true;
		if( _is_long || _t._is_long ) return false;

		if( _is_float && _t._is_float ) return true;
		if( _is_float || _t._is_float ) return false;

		if( _is_double && _t._is_double ) return true;
		if( _is_double || _t._is_double ) return false;


		if( _is_string && _t._is_string ) return true;
		if( _is_string || _t._is_string ) return false;

		if( _is_container != _t._is_container ) return false;
		if( _container_class != _t._container_class ) return false;

		return _class.equals(_t._class);
	}

	/**
	 * Checks for type to be the same, even if both are unknown.
	 * 
	 * @param o type to compare with
	 * @return true if types are the same
	 */
	public boolean equalsEvenUnknown( PhantomType o )
	{
		if( (!_is_known) && (!o._is_known) ) return true;
		return equals(o);		
	}
	
	public String toString()
	{
		String type = "unknown";
		if( _is_known )
		{
			if (_is_void)             type = "void";
			else if (_is_string)      type = "string";
			else if (_is_int)         type = "int";
			else if (_is_long)        type = "long";
			else if (_is_float)       type = "float";
			else if (_is_double)      type = "double";
			else if( _class != null ) type = _class.toString();
		}

		return
				type +
				(_is_container ?
						"["+(_container_class == null ? "" : "*"+_container_class.toString())+"]"
						: "");
	}

	public void emit_get_class_object( Codegen c, CodeGeneratorState s ) throws PlcException, IOException 
	{
		if(is_void() && !is_container())        //c.emit_summon_null();
			throw new PlcException("PhantomType","asked to emit void class object"); // Why not, btw?	
		
		else if(is_container())
		{
			if( _container_class != null )
				c.emitSummonByName(_container_class.getName());
			else if(_container_class_expression != null)
				_container_class_expression.generate_code(c,s);
			else
				c.emitSummonByName(DEFAULT_CONTAINER_CLASS); // TODO use class summon shortcuts!
		}
		else if(is_int())    c.emitSummonByName(".internal.int"); // TODO use class summon shortcuts!
		else if(is_long())   c.emitSummonByName(".internal.long");
		else if(is_float())  c.emitSummonByName(".internal.float");
		else if(is_double()) c.emitSummonByName(".internal.double");
		else if(is_string()) c.emitSummonByName(".internal.string");
		else if(_class != null) c.emitSummonByName(_class.getName());
		else if(_class_expression != null) _class_expression.generate_code(c,s);
		else
			throw new PlcException("emit_get_class_object","can't get class object");
	}


	// ------------------------------------------------------------------------
	// I/O
	// ------------------------------------------------------------------------

	public void save_to_file(RandomAccessFile os) throws IOException {
		Fileops.put_int32( os, is_container() ? 1 : 0);
		Fileops.put_string_bin( os, get_main_class_name() );
		Fileops.put_string_bin( os, get_contained_class_name() );
	}

	private void load(PhantomTypeInfo ti) throws IOException, PlcException {	
		if(ti.isContainer())
		{
			if(!(ti.getContainedClassName().equals("")))
			{
				_class = new PhantomClass(ti.getContainedClassName());
				_is_known = true;
			}
			if(!(ti.getMainClassName().equals("")))
			{
				_container_class = new PhantomClass(ti.getMainClassName());
				_is_known = true;
			}
		}
		else
		{
			if(!(ti.getMainClassName().equals("")))
			{
				_class = new PhantomClass(ti.getMainClassName());
				_is_known = true;
			}
		}

		
		// TODO separate this check in a method
		String main_class_name= ti.getMainClassName();
		
		_is_void   = main_class_name.equals(".internal.void");
		_is_int    = main_class_name.equals(".internal.int");
		_is_long   = main_class_name.equals(".internal.long");
		_is_float  = main_class_name.equals(".internal.float");
		_is_double = main_class_name.equals(".internal.double");
		_is_string = main_class_name.equals(".internal.string");
	}

	public PhantomType(PhantomTypeInfo ti) throws IOException, PlcException {
		load(ti);
	}

	public PhantomType(RandomAccessFile is) throws IOException, PlcException {
		load(new PhantomTypeInfo(is));
	}


	/**
	 * 
	 * @return Class of this type
	 */
	public PhantomClass get_class()
	{
		if( _class != null ) return _class;
		if( _is_int ) _class = ClassMap.get_map().get(".internal.int",false, null);
		if( _is_long ) _class = ClassMap.get_map().get(".internal.long",false, null);
		if( _is_float ) _class = ClassMap.get_map().get(".internal.float",false, null);
		if( _is_double ) _class = ClassMap.get_map().get(".internal.double",false, null);
		if( _is_string ) _class = ClassMap.get_map().get(".internal.string",false,null);

		if( _is_void ) 
			_class = ClassMap.get_map().get(".internal.void",false,null);

		return _class;
	}

	// ------------------------------------------------------------------------
	// Types compatibility
	// ------------------------------------------------------------------------

	public boolean can_be_assigned_from( PhantomType src )
	{
		if( this.equals(src) ) return true;

		if( is_void() || src.is_void() ) return false;
		
		if( !_is_known ) return true; // variables of unknown type can eat anything
		if( !src._is_known ) return false; // i know my type and he doesn't

		if( _is_container != src._is_container ) return false;

		if( is_on_int_stack() && src.is_on_int_stack() )
			return true;

		if(get_class() == null) return true;

		return get_class().can_be_assigned_from( src.get_class() );
	}




	public static final PhantomType t_string = new PhTypeString();
	public static final PhantomType t_void = new PhTypeVoid();

	public static PhantomType getVoid()  { return t_void; } 
	public static PhantomType getString()  { return t_string; }

	private static PhantomType t_int = null;
	public static PhantomType getInt() { 
		if( t_int == null ) t_int = new PhTypeInt();
		return t_int;
	}


	private static PhantomType t_long = null;
	public static PhantomType getLong() {
		if( t_long == null ) t_long = new PhTypeLong();
		return t_long;
	}

	private static PhantomType t_float = null;
	public static PhantomType getFloat() {
		if( t_float == null ) t_float = new PhTypeFloat();
		return t_float;
	}

	private static PhantomType t_double = null;
	public static PhantomType getDouble() {
		if( t_double == null ) t_double = new PhTypeDouble();
		return t_double;
	}

	// ---------------------------- LLVM code generation ----------------------------

	public String toLlvmType() {
		if( _class != null ) return LlvmCodegen.getObjectType();  

		if( _is_int )     return "i32";
		if( _is_long )    return "i64";
		if( _is_float )   return "float";
		if( _is_double )  return "double";
		//if( _is_string ) _class = ClassMap.get_map().get(".internal.string",false,null);

		return "void";
	}

	public String to_C_Type() {
		if( _class != null ) return C_codegen.getObjectType();  

		if( _is_int )     return "int_32_t";
		if( _is_long )    return "int_64_t";
		if( _is_float )   return "float";
		if( _is_double )  return "double";
		//if( _is_string ) _class = ClassMap.get_map().get(".internal.string",false,null);

		return "void";
	} 

	public String toJavaType() {
		if( _class != null ) return "Object"; // TODO must generate real class names  

		if( _is_int )     return "int";
		if( _is_long )    return "long";
		if( _is_float )   return "float";
		if( _is_double )  return "double";
		//if( _is_string ) _class = ClassMap.get_map().get(".internal.string",false,null);

		return "void";
	}


	// This is used to generate function name with encoded arg type info
	public String toProxyName() {
		if( _class != null ) return "o";
		return toLlvmType();
	}

	private static PhantomType t_object = null;

	public static PhantomType getObject() throws PlcException {
		if( t_object  == null )
			t_object = new PhantomType( ClassMap.get_map().get(".internal.object",false, null) );
		return t_object;
	}

	public static PhantomType findAbbreviatedType(String tn, boolean is_container) throws PlcException {

		// short type names

		if( tn.startsWith(".") )
			tn = tn.substring(1);

		switch(tn)
		{
		case "internal.object": 
			return PhantomType.getObject();

		case "void": return PhantomType.getVoid(); 

		case "internal.int": 
		case "int": 
			return PhantomType.getInt();

		case "internal.long":
		case "long": 
			return PhantomType.getLong(); 

		case "internal.float":
		case "float": 
			return PhantomType.getFloat();

		case "internal.double":
		case "double": 
			return PhantomType.getDouble();

		case "internal.string":			
		case "string": 
			return PhantomType.getString(); 
		}

		return null;
	}

	/**
	 * 
	 * @param b
	 * @return true if I am bigger or equal type (must be result type if combined with b )
	 */
	public boolean isBigger( PhantomType b )
	{
		if( _is_double ) return true;
		if( b._is_int ) return true;
		if( b._is_long && _is_float ) return true;
		return false;
	}

	static public PhantomType biggerType( PhantomType a, PhantomType b )
	{
		return a.isBigger(b) ? a : b; 
	}

	public static PhantomType findCompatibleType(PhantomType tl, PhantomType tr) {
		// simple
		if( tl.equals(tr) )
			return tl;

		if( (!tl.is_on_int_stack()) || (!tr.is_on_int_stack()) )
		{
			return null;
		}

		return biggerType(tl, tr);
	}

	//public void setStatic(boolean iAmStatic) { this.iAmStatic = iAmStatic; }
	public boolean getStatic() { return iAmStatic; }

	public PhantomType toStatic()
	{
		PhantomType ret = new PhantomType(this);
		ret.iAmStatic = true;
		return ret;
	}


	/**
	 * Do we represent container class with non-default container implementation
	 * @return True if so.
	 */
	public boolean isSpecificContainerClass() {		return _container_class != null ;	}

}








