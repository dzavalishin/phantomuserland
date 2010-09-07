package ru.dz.pfsck;

import java.nio.MappedByteBuffer;

//public class Phantom_File_System_Checker
//{
//}

public class Phantom_FS_Image
{
	private final MappedByteBuffer map;

	public Phantom_FS_Image(MappedByteBuffer map) {
		this.map = map;
	}

	
	//private BinaryReader m_Reader;

	public final int getBlockCount()
	{
		return map.limit() / ConstantProvider.DISK_STRUCT_BS;
		//return (int)(m_Reader.BaseStream.getLength() / ConstantProvider.DISK_STRUCT_BS);
	}



	//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public Block ReadBlock(System.UInt32 nBlock)
	public final Block ReadBlock(int nBlock)
	{
		//m_Reader.BaseStream.Seek(nBlock * ConstantProvider.DISK_STRUCT_BS, SeekOrigin.Begin);

		byte [] buf = new byte[ConstantProvider.DISK_STRUCT_BS];
		
		map.get(buf, nBlock * ConstantProvider.DISK_STRUCT_BS, ConstantProvider.DISK_STRUCT_BS);
		//MappedByteBuffer.wrap(buf);
		
		return new Block(buf);
	}

	public final Block ReadBlock()
	{
		byte [] buf = new byte[ConstantProvider.DISK_STRUCT_BS];	
		map.get(buf);		
		return new Block(buf);
	}

	//TODO: добавить параметр «Ожидаемый тип блока»
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public void linkedBlockList(System.UInt32 nBlock, ref ListDescriptor list)
	public final void linkedBlockList(int nBlock, RefObject<ListDescriptor> list)
	{
		if (list.argvalue == null)
		{
			list.argvalue = new ListDescriptor();
		}

		Block block = ReadBlock(nBlock);

		//if (block.Magic != ОжидаемыйТипБлока)
		//{
		//    throw new Exception("Ошибка: в цепи обнаружен блок типа " + block.Magic.ToString() + ", ожидался тип " + ОжидаемыйТипБлока.ToString());
		//}

		Phantom_Disk_List list_block = new Phantom_Disk_List(block);

		if (list.argvalue.contains(nBlock))
		{
			throw new RuntimeException("Ошибка: циклическая ссылка в связных блоках");
		}

		list.argvalue.add(nBlock);

		if (list_block.getNext() != 0)
		{
			linkedBlockList(list_block.getNext(), list);
		}
	}

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public BlockList blockListContents(System.UInt32 nBlock, out ListDescriptor BlocksWithList)
	public final BlockList blockListContents(int nBlock, RefObject<ListDescriptor> BlocksWithList)
	{
		BlockList blist = new BlockList();

		BlocksWithList.argvalue = null;

		{
			linkedBlockList(nBlock, BlocksWithList);

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: foreach (System.UInt32 blockWithList in BlocksWithList)
			for (int blockWithList : BlocksWithList.argvalue)
			{
				Block block = ReadBlock(blockWithList);

				Phantom_Disk_List list = new Phantom_Disk_List(block);

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: foreach (System.UInt32 blockNo in list.БлокиВСписке)
				for (int blockNo : list.getBlocksInList())
				{
					blist.add(blockNo);
				}
			}
		}

		return blist;
	}

	public final Phantom_Disk_Superblock getSuperBlock()
	{
		Block block = ReadBlock(ConstantProvider.DISK_STRUCT_SB_OFFSETS.First.getValue());

		Phantom_Disk_Superblock sblock = new Phantom_Disk_Superblock(block);

		return sblock;
	}
}