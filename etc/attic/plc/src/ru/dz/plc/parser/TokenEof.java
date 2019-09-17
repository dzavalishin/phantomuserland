package ru.dz.plc.parser;

import ru.dz.plc.util.*;

public class TokenEof extends Token {

  public boolean like(String part) { return false; }
  public boolean is(String part) throws PlcException { return false; }

  public Token emit(String src) throws PlcException { return this; }

  public String toString() { return "EOF"; }

  public TokenEof() { super(0); }

  public boolean is_eof() { return true; }
}

