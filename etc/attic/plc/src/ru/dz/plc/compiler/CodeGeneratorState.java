package ru.dz.plc.compiler;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 *
 * <p>Description: code generator state - passed to nodes which will use it to
 * track process state</p>
 *
 * <p>Copyright: Copyright (c) 2004-2009</p>
 *
 * <p>Company: Digital Zone</p>
 *
 * @author dz
 * @version 1.0
 */

public class CodeGeneratorState extends GeneralState {
  public String continue_label = null;
  public String break_label = null;

  public CodeGeneratorState(PhantomClass c)
  {
    super(c);
  }
}

