package ru.dz.plc.parser;

import ru.dz.plc.util.*;

public class Keyword
    extends Token {
  int id;
  String text;
  public Keyword(int id, String text) {
    super(id);
    //this.id = id;
    this.text = text;
  }

  public String get_text() { return text;  }
  public String toString() { return text;  }

  public boolean like(String part) {
    if( part.length() > text.length() ) return false;
    return part.equals(text.substring(0, part.length()));
  }

  public boolean is(String part) {
    return part.equals(text);
  }

  public Token emit(String src) throws PlcException {
    if (!src.equals(text))throw new PlcException("Lex",
        "emit is called with different text", src);
    return this;
  }
}
