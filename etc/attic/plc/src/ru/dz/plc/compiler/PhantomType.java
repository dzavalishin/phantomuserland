package ru.dz.plc.compiler;

import ru.dz.phantom.code.*;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.*;

import java.io.*;

/**
 * <p>Type. Class + possible container attributes.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class PhantomType {
	protected PhantomClass  _class;
	protected PhantomClass  _container_class;
	boolean             _is_void;
	boolean             _is_container;
	boolean             _is_known;
	boolean             _is_int;
	boolean             _is_string;

	Node                _class_expression;
	Node                _container_class_expression;

	public String get_main_class_name()
	{
		if(_is_container)  return _container_class == null ? "" : _container_class.getName();
		if(_is_void)       return ".internal.void";
		if(_is_int)        return ".internal.int";
		if(_is_string)     return ".internal.string";
		return _class == null ? "" : _class.getName();
	}

	public String get_contained_class_name()
	{
		if(_is_container)
			return _class == null ? "" : _class.getName();
		return "";
	}
	//public PhantomType() {_void = true; _is_container = false; _class = null; } // void
	protected PhantomType()
	{
		_is_void = false;
		_is_string = false;
		_is_int = false;
		_is_container = false;
		_is_known = true;
		_class = null;
		_container_class = null;
	}

	public PhantomType(PhantomClass _c) throws PlcException
	{
		//if( _c == null )			throw new PlcException("PhantomType(PhantomClass)","Class is null");
		
		_is_void = false;
		_is_string = false;
		_is_int = false;
		_is_container = false;
		_is_known = (_c != null);
		_class = _c;
		_container_class = null;

		if( _c != null && _c.getName().equals(".internal.int") ) _is_int = true;
		if( _c != null && _c.getName().equals(".internal.string") ) _is_string = true;
		if( _c != null && _c.getName().equals(".internal.void") ) _is_void = true;
		//if( _c != null && _c.get_name().equals(".internal.object") ) _is_void = true;
	}

	public PhantomType(PhantomClass _c, boolean is_container )
	{
		_is_void = false;
		_is_string = false;
		_is_int = false;
		_is_known = true;
		_is_container = is_container;
		_class = _c;
		_container_class = null;
	}

	public boolean is_void() { return _is_void; }
	public boolean is_int() { return _is_int; }
	public boolean is_string() { return _is_string; }
	public boolean is_container() { return _is_container; }
	public boolean is_unknown() { return !_is_known; }

	public boolean equals( Object o )
	{
		if( o == null || ! (o instanceof PhantomType ) ) return false;
		
		PhantomType _t = (PhantomType)o;

		if( !_is_known || !_t._is_known ) return false;

		if( _is_void && _t._is_void ) return true;
		if( _is_void || _t._is_void ) return false;

		if( _is_int && _t._is_int ) return true;
		if( _is_int || _t._is_int ) return false;

		if( _is_string && _t._is_string ) return true;
		if( _is_string || _t._is_string ) return false;

		if( _is_container != _t._is_container ) return false;
		if( _container_class != _t._container_class ) return false;

		return _class.equals(_t._class);
	}

	public String toString()
	{
		String type = "unknown";
		if( _is_known )
		{
			if (_is_void)             type = "void";
			else if (_is_string)      type = "string";
			else if (_is_int)         type = "int";
			else if( _class != null ) type = _class.toString();
		}

		return
		type +
		(_is_container ?
				"["+(_container_class == null ? "" : "*"+_container_class.toString())+"]"
				: "");
	}

	public void emit_get_class_object( Codegen c, CodeGeneratorState s ) throws PlcException, IOException {
		if(is_void() && !is_container())        //c.emit_summon_null();
			throw new PlcException("PhantomType","asked to emit void class object"); // Why not, btw?
		else if(is_int())    c.emitSummonByName(".internal.int");
		else if(is_string()) c.emitSummonByName(".internal.string");
		else if(is_container())
		{
			if( _container_class != null )
				c.emitSummonByName(_container_class.getName());
			else if(_container_class_expression != null)
				_container_class_expression.generate_code(c,s);
			else
				c.emitSummonByName(".internal.container.array");
		}
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

	public PhantomType(RandomAccessFile is) throws IOException, PlcException {
		_class_expression = _container_class_expression = null;
		_class = _container_class = null;
		_is_void = _is_known = _is_int = _is_string = false;

		boolean _is_container = Fileops.get_int32(is) != 0;
		String main_class_name = Fileops.get_string(is);
		String contained_class_name = Fileops.get_string(is);


		if(_is_container)
		{
			if(!(contained_class_name.equals("")))
			{
				_class = new PhantomClass(contained_class_name);
				_is_known = true;
			}
			if(!(main_class_name.equals("")))
			{
				_container_class = new PhantomClass(main_class_name);
				_is_known = true;
			}
		}
		else
		{
			if(!(main_class_name.equals("")))
			{
				_class = new PhantomClass(main_class_name);
				_is_known = true;
			}
		}

		_is_void   = main_class_name.equals(".internal.void");
		_is_int    = main_class_name.equals(".internal.int");
		_is_string = main_class_name.equals(".internal.string");
	}

	/**
	 * 
	 * @return Class of this type
	 */
	public PhantomClass get_class()
	{
		if( _class != null ) return _class;
		if( _is_int ) _class = ClassMap.get_map().get(".internal.int",false, null);
		if( _is_string ) _class = ClassMap.get_map().get(".internal.string",false,null);

		return _class;
	}

	// ------------------------------------------------------------------------
	// Types compatibility
	// ------------------------------------------------------------------------

	public boolean can_be_assigned_from( PhantomType src )
	{
		if( this.equals(src) ) return true;

		if( !_is_known ) return true; // variables of unknown type can eat anything
		if( !src._is_known ) return false; // i know my type and he doesnt

		if( _is_void || src._is_void ) return false;

		if( _is_container != src._is_container ) return false;

		if(get_class() == null) return true;
		
		return get_class().can_be_assigned_from( src.get_class() );
	}

}








