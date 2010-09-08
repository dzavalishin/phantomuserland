package ru.dz.pfsck;

import java.util.AbstractList;

public class Phantom_Disk_List extends Block
{
	
	public Phantom_Disk_List(Block block) {
		map = block.map.duplicate();
		map.order(Program.BYTE_ORDER);
	}

	//[DescriptionAttribute("num of first unused slot in list")]

	public final int getUsed()
	{
		return BitConverter.ToInt32(map, 4);
	}

	public final int getNext()
	{
		return BitConverter.ToInt32(map, 8);
	}


	public final AbstractList<Integer> getBlocksInList()
	{
//ORIGINAL LINE: List<System.UInt32> blocks = new List<uint>();
		java.util.ArrayList<Integer> blocks = new java.util.ArrayList<Integer>();

//ORIGINAL LINE: System.UInt32 n = Used;
		int n = getUsed();

//ORIGINAL LINE: for (System.UInt32 i = 0; i < n; i++)
		for (int i = 0; i < n; i++)
		{
//ORIGINAL LINE: System.UInt32 НомерБлока = BitConverter.ToUInt32(m_Buffer, (int)(16 + (4 * i)));
			int blockNo = BitConverter.ToInt32(map, (int)(16 + (4 * i)));

			blocks.add(blockNo);
		}

		return blocks;
	}
}

