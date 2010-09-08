package ru.dz.pfsck;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;

public class Block
{
	ByteBuffer map;

	//[Category("1. Общий"), DescriptionAttribute("Magic")]
	public final Magics getMagic()
	{
		//map.position(0);
		//return (Magics)BitConverter.ToInt32(m_Buffer, 0);
		return Magics.forValue( map.getInt(0) );
	}

	public Block()
	{
	}

	public Block(byte[] buffer)
	{
		map = MappedByteBuffer.wrap(buffer);
		map.order(Program.BYTE_ORDER);
	}

	public Block(InputStream reader)
	{
		byte[] m_Buffer = new byte[ConstantProvider.DISK_STRUCT_BS];
		try {
			if( reader.read(m_Buffer) != ConstantProvider.DISK_STRUCT_BS )
				throw new RuntimeException("Can' read block");
		} catch (IOException e) {
			throw new RuntimeException("Can' read block", e);
		}
		
		map = MappedByteBuffer.wrap(m_Buffer);
		map.order(Program.BYTE_ORDER);
	}
}

