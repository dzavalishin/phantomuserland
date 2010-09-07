package ru.dz.pfsck;

import java.nio.ByteBuffer;

public class BitConverter {

	public static int ToInt32(ByteBuffer buffer, int pos) {
		return buffer.getInt(pos);
	}

	public static int ToUInt32(ByteBuffer buffer, int pos) {
		return buffer.getInt(pos);
	}

}
