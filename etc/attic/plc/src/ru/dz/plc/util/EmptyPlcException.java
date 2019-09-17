package ru.dz.plc.util;

public class EmptyPlcException extends PlcException {
  public EmptyPlcException( String where, String param )
  {
    super(where, "nothing found", param );
  }

  public EmptyPlcException( String where )
  {
    super(where, "nothing found", null );
  }
}
