package ru.dz.plc.compiler;

/**
 * <p>Argument definition.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class ArgDefinition
{
  private String   name;
  private PhantomType  type;

  public ArgDefinition( String name, PhantomType type )
  {
    this.name = name;
    this.type = type;
  }

  public String getName() { return name; }
  public PhantomType getType() { return type; }

  public String toString()
  {
    return
        (name == null ? "(null name)" : name)
        +
        ": "
        +
        (type == null ? "(null)" : type.toString());
  }




};

