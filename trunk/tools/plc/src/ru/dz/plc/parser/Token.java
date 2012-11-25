package ru.dz.plc.parser;

import ru.dz.plc.util.*;

public abstract class Token {
  int id;

  public abstract boolean like(String part);
  public abstract boolean is(String part) throws PlcException;

  public abstract Token emit(String src) throws PlcException;

  public abstract String toString();
  public          String value() { return null; }

  public Token( int id ) { this.id = id; }
  public int get_id() {      return id;    }

  public boolean is_eof() { return false; }
}

