package ru.dz.plc.parser;

import ru.dz.plc.util.*;

public class TokenIdent extends Token {

  public TokenIdent( int id ) { super(id); }

  boolean first_char(char c) {
    //Character cc = new Character(c);
    return Character.isJavaIdentifierStart(c);
  }

  boolean next_char(char c) {
    if( first_char(c) ) return true;
    if( c < '0' || c > '9' )  return false;
    return true;
  }


  public boolean like( String part ) {
    if( first_char(part.charAt(0)) == false ) return false;

    for( int i = 1; i < part.length(); i++ ) {
      char c = part.charAt(i);
      if( next_char(c) == false )             return false;
    }
    return true;
  }

  public boolean is( String part ) { return like(part ); }

  public Token emit( String src ) throws PlcException {
    if( !like( src ) ) throw new PlcException( "Lex", "ident::emit is called with not an ident", src );

    return new token_ident_val( id, src );
  }
  public String toString() { return "generic ident"; }
}

class token_ident_val extends TokenIntConst {
  String val;
  public token_ident_val( int id, String src ) {
    super(id);
    // remove "" around
    //val.getInteger(src.substring(1, src.length() - 2));
    val = src;
  }
  public String value() { return val; }
  public String toString() { return val; }
}
