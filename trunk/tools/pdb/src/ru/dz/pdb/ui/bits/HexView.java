package ru.dz.pdb.ui.bits;

public class HexView {










	
	static final String HEXCHARS[] = {
		"0", "1", "2", "3", "4", "5", "6", "7", 
		"8", "9", "A", "B", "C", "D", "E", "F"
		};

	
	/**
	 * Convert a byte[] array to hex dump.
	 * 
	 * @return result String buffer in String format 
	 * @param in byte[] buffer to convert to string format
	 */

	static String byteArrayToHexString( byte in[] ) 
	{
		byte ch;
		int i; 

		if (in == null || in.length <= 0)
			return null;

		StringBuffer out = new StringBuffer(in.length * 3 + 2);



		for(i = 0; i < in.length; i++) 
		{

			ch = (byte) ((in[i] >> 4) & 0x0F);
			out.append(HEXCHARS[ (int) ch]); // convert the			nibble to a String Character

			ch = (byte) (in[i] & 0x0F); // Strip off			low nibble 
			out.append(HEXCHARS[ (int) ch]); // convert the			nibble to a String Character
		}

		out.append(' ');
		
		for(i = 0; i < in.length; i++) 
		{
			out.append((char)in[i]);
		}
		
		return out.toString();
	}    


}
