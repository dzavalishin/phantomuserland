package ru.dz.plc.parser;

public class TokenIntConstVal extends TokenIntConst {
  Integer val;
  public TokenIntConstVal( int id, String src ) {
    super(id);
    //val.getInteger(src);
    val = new Integer(src);
  }
  public int int_value() { return val.intValue(); }
  public String value() { return val.toString(); }
  public String toString() { return val.toString(); }
}
