package ru.dz.plc.parser;

import ru.dz.plc.util.*;

public class TokenIntConst
extends Token {
	
	public TokenIntConst(int id ) { super(id); }

	public boolean like(String part) 
	{
		int i = 0;
		boolean hex = false;
		
		//System.out.format("number '%s'  ", part);
		// have to do it in grammar, or else 2-2 can't be compiled
		//if( part.charAt(0) == '-') part = part.substring(1);
		
		
		
		// Hex const - if part len is 1, leading 0 will be covered below in for loop
		if( (part.length() >= 2) && (part.charAt(0) == '0') && (part.charAt(1) == 'x') )
		{
			i += 2;
			
			if( part.length() <= 2)				
				return true;
			
			hex = true;
		}
		
		for (; i < part.length(); i++) 
		{
			char c = part.charAt(i);
			
			if( hex )
			{
				if( (c >= 'a') && (c <= 'f') )
					continue;
				if( (c >= 'A') && (c <= 'F') )
					continue;
			}
			
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



