package ru.dz.plc.compiler;

import java.util.*;

import ru.dz.phantom.code.*;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

import java.io.IOException;

/**
 * <p>Represents a method.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class Method
{
  public int          ordinal;
  public Node         code;
  public String       name;
  public PhantomType  type;
  //public Node         args;

  // Main ones - on object stack
  public PhantomStack     svars = new PhantomStack();
  // Binary ones - on integer stack
  public PhantomStack     isvars = new PhantomStack();

  public LinkedList<ArgDefinition>   args_def = new LinkedList<ArgDefinition>();

  Codegen c = new Codegen();


  /*
  Method( String name, PhantomType type, Node args, int n, Node c )
  {
    this.name = name;
    this.type = type;
    this.args = args;
    ordinal = n;
    code = c;
  }

  Method( String name, PhantomType type, int ordinal, Node code )
  {
    this.name = name;
    this.type = type;
    this.args = null;
    this.ordinal = ordinal;
    this.code = code;
  }*/

  public Method( String name, PhantomType type )
  {
    this.name = name;
    this.type = type;
    //this.args = null;
    this.ordinal = -1;
    this.code = null;
  }

  /** NB! Must be called in correct order! */
  public void addArg( String name, PhantomType type ) throws PlcException
  {
    if( type == null )
      {
        //Object a = null;        a.getClass();
        throw new PlcException("Method add_arg", "null type of arg", name);
      }

    args_def.add(new ArgDefinition(name, type));
    svars.add_stack_var( new PhantomVariable(name, type ) );
  }

  public String toString()
  {
    StringBuffer sb = new StringBuffer();

    sb.append( type.toString() );
    sb.append( " " );
    sb.append( name );
    sb.append( "( " );
    boolean first = true;
    for( Iterator<ArgDefinition> i = args_def.iterator(); i.hasNext(); )
    {
      if( !first ) sb.append(", ");
      first = false;
      sb.append(i.next().toString());
    }
    sb.append( " )" );

    return sb.toString();
  }

  public Codegen get_cg() { return c; }

  public void generate_code( CodeGeneratorState s ) throws PlcException, IOException {
    if(code == null)
    {
      c.emitRet(); // empty function code
      return;
    }

    boolean enable_debug = false;
    //enable_debug = true;

    // ------------------------------------------
    // traverse tree to allocate automatic vars?
    // ------------------------------------------
    int n_auto_vars = svars.getUsedSlots();
    int n_int_auto_vars = isvars.getUsedSlots();
    // ------------------------------------------

    // ------------------------------------------
    // generate prologue code here
    // ------------------------------------------

    // check number of args
    int n_args = args_def.size();
    String good_args_label = c.getLabel();
    c.emitIConst_32bit(n_args);
    c.emitISubLU();
    c.emitJz(good_args_label);

    // wrong count - throw string
    // BUG! Need less consuming way of reporting this. Maybe by
    // calling class object Method? Or by summoning something?
    c.emitString("arg count: "+name + " in " + s.my_class.getName());
    c.emitThrow();

    c.markLabel(good_args_label);

    if(enable_debug) c.emitDebug((byte)0x1,"Enabled debug");

    // push nulls to reserve stack space for autovars
    // BUG! We can execute vars initialization code here, can we?
    // We can if code does not depend on auto vars itself, or depends only on
    // previous ones.
    // TODO: BUG! autovars in arguments are already on stack, we don't have to reserve space
    for( int i = n_auto_vars; i > 0; i-- )
      c.emitPushNull();

    // TODO introduce instruction to reserve o+i space in one step
    // Reserve integer stack place for int vars
    for( int i = n_int_auto_vars; i > 0; i-- )
    	c.emitIConst_0();
    	
    // ------------------------------------------


    // ------------------------------------------
    // generate main code by descending the tree
    // ------------------------------------------
    code.generate_code( c, s );
    // ------------------------------------------

    // ------------------------------------------
    // generate epilogue code here
    // ------------------------------------------
    if(enable_debug) c.emitDebug((byte)0x2,"Disabled debug");
    c.emitRet(); // catch the fall-out
    // ------------------------------------------

  }

public int getOrdinal() {
	if(ordinal < 0)
		System.out.println("Method.getOrdinal(): ordinal < 0");
	return ordinal; 
	}


};












