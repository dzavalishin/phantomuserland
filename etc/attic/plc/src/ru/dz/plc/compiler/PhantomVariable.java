package ru.dz.plc.compiler;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: </p>
 * @author dz
 * @version 1.0
 */

public class PhantomVariable
{
  String   name;
  PhantomType  type;

  public PhantomVariable( PhantomVariable v )
  {
    this.name = v.name;
    this.type = v.type;
  }

  public PhantomVariable( String name, PhantomType type)
  {
    this.name = name;
    this.type = type;
  }

  public String getName() { return name; }
  public PhantomType  getType() { return type; }


};
