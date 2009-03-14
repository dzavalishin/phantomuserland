package ru.dz.plc.compiler;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: </p>
 * @author dz
 * @version 1.0
 */

public class PhantomField extends PhantomVariable
{
  int      ordinal;

  PhantomField( String name, PhantomType type , int n)
  {
    super( name, type );
    ordinal = n;
  }

  public int      get_ordinal() { return ordinal; }

};
