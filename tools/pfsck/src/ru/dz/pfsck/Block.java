package ru.dz.pfsck;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;

public class Block
{
	//public byte[] m_Buffer;
	ByteBuffer map;

//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
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
		//m_Buffer = buffer;
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
	}
}

