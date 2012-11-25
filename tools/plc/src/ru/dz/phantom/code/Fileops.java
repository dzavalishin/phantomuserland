package ru.dz.phantom.code;

import java.io.*;

/**
 * <p>Binary conversions. To replace with ByteBuffer, sure. :)</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class Fileops
{
	// ------------------------------------------------------------------------
	// Read
	// ------------------------------------------------------------------------

	public static int get_int32( RandomAccessFile is ) throws IOException {
		byte bb[] = new byte[4];

		is.read(bb);

		int v;

		v = bb[3];
		v |= ((int)bb[2]) << 8;
		v |= ((int)bb[1]) << 16;
		v |= ((int)bb[0]) << 24;
		return (int)v;
	}

    public static byte[] get_bytes(RandomAccessFile is, int length) throws IOException {
        byte bb[] = new byte[length];

        is.read(bb);
        return bb;
    }

	public static String get_string( RandomAccessFile is ) throws IOException {
		int len = get_int32(is);

		byte data[] = new byte[len];
		is.read(data);

		return new String(data);
	}


	// ------------------------------------------------------------------------
	// Wrire
	// ------------------------------------------------------------------------

	public static void put_byte( RandomAccessFile os, byte v ) throws IOException {
		byte[] bb = new byte[1];
		bb[0] = v;
		os.write(bb);
	}

	public static void put_int32( RandomAccessFile os, int v ) throws IOException {
		byte[] bb = new byte[4];
		bb[3] = (byte)( v        & 0xFF);
		bb[2] = (byte)((v >>  8) & 0xFF);
		bb[1] = (byte)((v >> 16) & 0xFF);
		bb[0] = (byte)((v >> 24) & 0xFF);
		os.write(bb);
	}

	public static void put_int64( RandomAccessFile os, long v ) throws IOException 
	{
		byte[] bb = new byte[8];
		
		bb[7] = (byte)( v        & 0xFF);
		bb[6] = (byte)((v >>  8) & 0xFF);
		bb[5] = (byte)((v >> 16) & 0xFF);
		bb[4] = (byte)((v >> 24) & 0xFF);

		bb[3] = (byte)((v >> 32) & 0xFF);
		bb[2] = (byte)((v >> 40) & 0xFF);
		bb[1] = (byte)((v >> 48) & 0xFF);
		bb[0] = (byte)((v >> 56) & 0xFF);

		os.write(bb);
	}

	/*
  static public void put_string_zero( RandomAccessFile os, String v ) throws IOException {
    Charset cs = Charset.forName("UTF-8");
    ByteBuffer bb = cs.encode(v);
    os.write(bb.array());
    os.write(0);
    os.write(0); // BUG? UTF-8 strings should end with one or two zeros?
  }
	 */

	public static void put_string_bin( RandomAccessFile os, byte[] v ) throws IOException {
		put_int32( os, v.length );
		os.write(v);
	}

	public static void put_string_bin( RandomAccessFile os, String v ) throws IOException {
		//Charset cs = Charset.forName("UTF-8");
		//ByteBuffer bb = cs.encode(v);
		//put_string_bin( bb.array() );
		// BUG! must be above version for UTF-8!
		// but does not compile with gcj
		put_string_bin( os, v.getBytes() );
	}


}
