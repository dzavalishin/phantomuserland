package ru.dz.plc.parser;

public class TokenIntConstVal extends TokenIntConst {
	
	private Long val;
	
	public TokenIntConstVal( int id, String src ) {
		super(id);

		boolean neg = false;

		if( src.charAt(0) == '-')
		{
			neg = true;
			src = src.substring(1);
		}

		if( (src.length() >= 2) && (src.charAt(0) == '0') && (src.charAt(1) == 'x') )
		{
			val = Long.parseLong(src.substring(2), 16);
			//val = Integer.parseInt(src, 16);
		}
		else if( src.charAt(0) == '0' )
		{
			val = Long.parseLong(src, 8);
		}
		else
		{
			//val.getInteger(src);
			val = new Long(src);
		}
		
		if(neg) val =-val;
	}
	public int int_value() { return val.intValue(); }
	public String value() { return val.toString(); }
	public String toString() { return val.toString(); }
}
