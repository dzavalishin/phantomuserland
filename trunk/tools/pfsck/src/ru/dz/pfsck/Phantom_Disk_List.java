public class Phantom_Disk_List extends Block
{
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("num of first unused slot in list")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public System.UInt32 getUsed()
	public final int getUsed()
	{
//C# TO JAVA CONVERTER TODO TASK: There is no Java equivalent to 'sizeof':
		return BitConverter.ToUInt32(m_Buffer, sizeof(int));
	}

//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("next list page in a chain or 0")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getNext()
	public final int getNext()
	{
		return BitConverter.ToUInt32(m_Buffer, 8);
	}

	public Phantom_Disk_List(byte[] buffer)
	{
		m_Buffer = buffer;
	}

	public final java.util.ArrayList<Integer> getBlocksInList()
	{
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: List<System.UInt32> blocks = new List<uint>();
		java.util.ArrayList<Integer> blocks = new java.util.ArrayList<Integer>();

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: System.UInt32 n = Used;
		int n = getUsed();

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: for (System.UInt32 i = 0; i < n; i++)
		for (int i = 0; i < n; i++)
		{
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: System.UInt32 НомерБлока = BitConverter.ToUInt32(m_Buffer, (int)(16 + (4 * i)));
			int blockNo = BitConverter.ToUInt32(m_Buffer, (int)(16 + (4 * i)));

			blocks.add(blockNo);
		}

		return blocks;
	}
}