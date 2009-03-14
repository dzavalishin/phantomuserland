package ru.dz.plc.parser;

import ru.dz.plc.util.*;

public class TokenIntConst
    extends Token {
  public TokenIntConst(int id ) { super(id); }

  public boolean like(String part) {
    for (int i = 0; i < part.length(); i++) {
      char c = part.charAt(i);
      if ( !Character.isDigit(c) ) return false;
    }
    return true;
  }

  public boolean is(String part) { return like(part); }

  public Token emit(String src) throws PlcException {
    if (!like(src)) throw new PlcException("Lex",  "int_const::emit is called with NAN", src);

    return new TokenIntConstVal(id,src);
  }
  public String toString() { return "generic int"; }
}



