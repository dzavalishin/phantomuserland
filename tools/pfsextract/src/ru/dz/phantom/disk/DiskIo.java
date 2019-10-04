package ru.dz.phantom.disk;

import java.io.IOException;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;

public class DiskIo {
	private ByteBuffer bb = ByteBuffer.allocate(Const.DISK_STRUCT_BS);
	private FileChannel ifc;

	public DiskIo(FileChannel ifc) {
		this.ifc = ifc;
	}

	public void readBlock() throws IOException {
		bb.position(0);
		ifc.read(bb);
		bb.order(ByteOrder.LITTLE_ENDIAN);
	}
	
	public void readBlock(int startBlk) throws IOException 
	{
		ifc.position(startBlk * Const.DISK_STRUCT_BS);
		readBlock();
	}

	
	
	public final int position() {		return bb.position();	}

	public final Buffer position(int newPosition) {		return bb.position(newPosition);	}

	public char getChar() {		return bb.getChar();	}
	public char getChar(int index) {		return bb.getChar(index);	}
	public short getShort() {		return bb.getShort();	}
	public short getShort(int index) {		return bb.getShort(index);	}
	public int getInt() {		return bb.getInt();	}
	public int getInt(int index) {		return bb.getInt(index);	}
	public long getLong() {		return bb.getLong();	}
	public long getLong(int index) {		return bb.getLong(index);	}
	public float getFloat() {		return bb.getFloat();	}
	public float getFloat(int index) {		return bb.getFloat(index);	}
	public double getDouble() {		return bb.getDouble();	}
	public double getDouble(int index) {		return bb.getDouble(index);	}

	public ByteBuffer get(byte[] dst, int offset, int length) {
		return bb.get(dst, offset, length);
	}

	public ByteBuffer get(byte[] dst) {
		return bb.get(dst);
	}

	public void write(FileChannel ofc) throws IOException {
		ofc.write(bb);		
	}


}
