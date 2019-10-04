package ru.dz.plc.util;

public class PlcException extends Throwable {
  String what = null, where = null, param = null;

  public PlcException( String where, String what, String param) {
    this.what = what; this.where = where; this.param = param;
  }

  public PlcException( String where, String what) {
    this.what = what; this.where = where; this.param = null;
  }

  public PlcException( String where ) {
    this.what = "(unknown)"; this.where = where; this.param = null;
  }


  public String toString() {
    String sub = what + " in " + where;
    return param == null ? sub : sub + " (" + param + ")";
  }
}

