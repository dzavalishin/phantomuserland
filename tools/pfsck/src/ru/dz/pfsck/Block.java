package ru.dz.pfsck;

public class Block
{
	public byte[] m_Buffer;

//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[Category("1. Общий"), DescriptionAttribute("Magic")]
	public final Magics getMagic()
	{
		return (Magics)BitConverter.ToInt32(m_Buffer, 0);
	}

	public Block()
	{
	}

	public Block(byte[] buffer)
	{
		m_Buffer = buffer;
	}

	public Block(BinaryReader reader)
	{
		m_Buffer = new byte[ConstantProvider.DISK_STRUCT_BS];
		reader.Read(m_Buffer, 0, ConstantProvider.DISK_STRUCT_BS);
	}
}

