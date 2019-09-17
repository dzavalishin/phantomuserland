package ru.dz.plc.parser;

import ru.dz.plc.util.*;

public class TokenStringConst extends Token {

  public TokenStringConst( int id ) { super(id); }

  static char conv_ctrl( char in ) throws PlcException {
    switch( in )
    {
        case '"': return '"';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'b': return '\7';
    }
    throw new PlcException("Lex", "wrong escape sequence", "\\"+in );
  }

  static boolean convert( StringBuffer out, String src ) throws PlcException {
    if( src.charAt(0) != '"' ) return false;
    int len = src.length() - 1;
    for( int i = 1; i < len; i++ )
    {
      if(src.charAt(i) == '"') return false;
      if(src.charAt(i) == '\\')
      {
        i++;
        if( i >= len ) break;
        if( out != null ) out.append( conv_ctrl(src.charAt(i)) );
      }
      else
        if( out != null ) out.append( src.charAt(i) );
    }

    if( src.charAt(len) != '"' ) return false;
    return true;
  }

  public boolean like( String src ) {
    if( src.charAt(0) != '"' ) return false;
    int len = src.length() - 1;
    for( int i = 1; i < len; i++ )
    {
      if(src.charAt(i) == '"') return false;
      if(src.charAt(i) == '\\')
      {
        i++;
//        if( i >= len ) return false;
      }
    }

    return true;
  }

  public boolean is( String src ) throws PlcException {
    return convert( null, src );
  }

  public Token emit( String src ) throws PlcException {
    if( !like( src ) ) throw new PlcException( "Lex", "ident::emit is called with not an ident", src );

    return new token_string_const_val( id, src );
  }
  public String toString() { return "generic string const"; }
}

class token_string_const_val extends TokenStringConst {
  String val;
  public token_string_const_val( int id, String src ) throws PlcException {
    super(id);
    // remove "" around
    //val.getInteger(src.substring(1, src.length() - 2));
    StringBuffer out = new StringBuffer(32);
    convert( out, src );
    val = out.toString();
  }
  public String value() { return val; }
  public String toString() { return "\""+val+"\""; }
}
