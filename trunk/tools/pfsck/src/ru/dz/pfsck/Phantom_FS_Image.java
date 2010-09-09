package ru.dz.pfsck;

import java.nio.MappedByteBuffer;
import java.util.AbstractList;
import java.util.logging.Level;
import java.util.logging.Logger;


public class Phantom_FS_Image
{
	Logger log = Logger.getLogger(Phantom_FS_Image.class.getName()); 
	
	private final MappedByteBuffer map;

	public Phantom_FS_Image(MappedByteBuffer map) {
		this.map = map;
	}

	

	public final int getBlockCount()
	{
		return map.limit() / ConstantProvider.DISK_STRUCT_BS;
		//return (int)(m_Reader.BaseStream.getLength() / ConstantProvider.DISK_STRUCT_BS);
	}



//ORIGINAL LINE: public Block ReadBlock(System.UInt32 nBlock)
	public final Block ReadBlock(int nBlock)
	{
		//log.log(Level.SEVERE, "reading block "+nBlock);
		//System.out.println("reading block "+nBlock);
		//m_Reader.BaseStream.Seek(nBlock * ConstantProvider.DISK_STRUCT_BS, SeekOrigin.Begin);

		byte [] buf = new byte[ConstantProvider.DISK_STRUCT_BS];
		//System.out.println("fs map limit = " + map.limit()/1024 + "kb" );
		
		map.position(nBlock * ConstantProvider.DISK_STRUCT_BS);
		map.get(buf, 0, ConstantProvider.DISK_STRUCT_BS);
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
//ORIGINAL LINE: public void linkedBlockList(System.UInt32 nBlock, ref ListDescriptor list)
	public final void loadLinkedBlockList(int nBlock, RefObject<ListDescriptor> list)
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
		
		AbstractList<Integer> blocksInList = list_block.getBlocksInList();
		
		if(Program.isVerbose())
		{
			System.out.println("Phantom_FS_Image.linkedBlockList() used = " + list_block.getUsed() );
			System.out.println("Phantom_FS_Image.linkedBlockList() = " + blocksInList );
		}
		list.argvalue.addAll(blocksInList);

		if (list.argvalue.contains(nBlock))
		{
			//throw new RuntimeException("Ошибка: циклическая ссылка в связных блоках, block no. "+nBlock+" list is "+list.argvalue);
			Program.reportError("cyclic link in block list, list page block no. "+nBlock+" list is "+list.argvalue);
			return;
		}

		list.argvalue.add(nBlock);

		if (list_block.getNext() != 0)
		{
			if(Program.isVerbose())
				System.out.println( "Phantom_FS_Image.linkedBlockList() next = " + list_block.getNext() );
			loadLinkedBlockList(list_block.getNext(), list);
		}
	}

//ORIGINAL LINE: public BlockList blockListContents(System.UInt32 nBlock, out ListDescriptor BlocksWithList)
	public final BlockList blockListContents(int nBlock, RefObject<ListDescriptor> BlocksWithList)
	{
		BlockList blist = new BlockList();

		BlocksWithList.argvalue = null;

		{
			loadLinkedBlockList(nBlock, BlocksWithList);

//ORIGINAL LINE: foreach (System.UInt32 blockWithList in BlocksWithList)
			for (int blockWithList : BlocksWithList.argvalue)
			{
				Block block = ReadBlock(blockWithList);

				Phantom_Disk_List list = new Phantom_Disk_List(block);

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

