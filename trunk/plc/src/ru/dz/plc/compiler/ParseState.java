package ru.dz.plc.compiler;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: </p>
 * @author dz
 * @version 1.0
 */

public class ParseState
    extends GeneralState {
  public ParseState(PhantomClass c)
  {
    super(c);
  }
  public ParseState()
  {
    super(null);
  }

  public void set_class ( PhantomClass c ) { my_class = c; }


}

