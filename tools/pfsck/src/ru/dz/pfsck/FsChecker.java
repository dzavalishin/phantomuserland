package ru.dz.pfsck;

import java.nio.MappedByteBuffer;


public class FsChecker 
{
	private Phantom_FS_Image m_Image;
	private final MappedByteBuffer map;
	private Phantom_Disk_Superblock superBlock;



	public FsChecker(MappedByteBuffer map) {
		this.map = map;
		map.load();
		
		//System.out.println("limit = " + map.limit() );
		System.out.println("limit = " + map.limit()/1024 + "kb" );
		
		m_Image = new Phantom_FS_Image(this.map);
		
		superBlock = m_Image.getSuperBlock();
	}
	

	public final void Check()
	{
		OutputLine("superblock");
		OutputLine("-----------------");
		Output(""+superBlock);

		if(superBlock.getBlocksize() != ConstantProvider.DISK_STRUCT_BS)
		{
			Program.reportFatalError("Blocksize is not "+ConstantProvider.DISK_STRUCT_BS);
			return;
		}
		
		if(superBlock.getVersionMajor() != 1)
		{
			Program.reportFatalError("Major version is unknown");
			return;
		}
		
		OutputLine("Begin check " + new java.util.Date().toString() + ".");
		OutputLine("-----------------");

		BlockList freeList = null;
		BlockList lastShotContentList = null;
		BlockList prevShotContentList = null;

		ListDescriptor freeSpaceListDescriptor = null;
		ListDescriptor lastShotListDescriptor = null;
		ListDescriptor prevShotListDescriptor = null;

		///#region Начальное заполнение списков
		{
			OutputLine("Loading lists");
			OutputLine("-----------------");

			if (superBlock.getFree_list() != 0)
			{
				RefObject<ListDescriptor> tempRef_freeSpaceListDescriptor = new RefObject<ListDescriptor>(freeSpaceListDescriptor);
				freeList = m_Image.blockListContents(superBlock.getFree_list(), tempRef_freeSpaceListDescriptor);
				freeSpaceListDescriptor = tempRef_freeSpaceListDescriptor.argvalue;

				OutputLine(" - Free list loaded");
				Output(freeSpaceListDescriptor);
				Output(freeList);
			}

			if (superBlock.getLast_snap() != 0)
			{
				RefObject<ListDescriptor> tempRef_lastShotListDescriptor = new RefObject<ListDescriptor>(lastShotListDescriptor);
				lastShotContentList = m_Image.blockListContents(superBlock.getLast_snap(), tempRef_lastShotListDescriptor);
				lastShotListDescriptor = tempRef_lastShotListDescriptor.argvalue;

				OutputLine(" - Last snap loaded");
				Output(lastShotListDescriptor);
				Output(lastShotContentList);
			}

			if (superBlock.getPrev_snap() != 0)
			{
				RefObject<ListDescriptor> tempRef_prevShotListDescriptor = new RefObject<ListDescriptor>(prevShotListDescriptor);
				prevShotContentList = m_Image.blockListContents(superBlock.getPrev_snap(), tempRef_prevShotListDescriptor);
				prevShotListDescriptor = tempRef_prevShotListDescriptor.argvalue;

				OutputLine(" - Prev snap loaded");
				Output(prevShotListDescriptor);
				Output(prevShotContentList);
			}

			OutputLine("Done");
		}
		///#endregion

		//TODO: проверить контрольную сумму суперблока

		//TODO: сравнить суперблоки

		///#region Проверка типов блоков в описателях списков
		OutputLine("");
		OutputLine("Check magics");
		OutputLine("-----------------");
		if (freeSpaceListDescriptor != null)
		{
			blockTypeCheck(freeSpaceListDescriptor, Magics.DISK_STRUCT_MAGIC_FREEHEAD);
		}
		if (lastShotListDescriptor != null)
		{
			blockTypeCheck(lastShotListDescriptor, Magics.DISK_STRUCT_MAGIC_SNAP_LIST);
		}
		if (prevShotListDescriptor != null)
		{
			blockTypeCheck(prevShotListDescriptor, Magics.DISK_STRUCT_MAGIC_SNAP_LIST);
		}
		OutputLine("Done");
		///#endregion

		//TODO: статистика пересечения снимков
		///#region Статистика пересечения снимков
		///#endregion

		///#region Чтоб списки не пересекались
		{
			java.util.HashMap<Integer, Boolean> t = new java.util.HashMap<Integer, Boolean>();

			OutputLine("");
			OutputLine("");
			OutputLine("Check for block being in more than one list");
			OutputLine("-----------------");

			if (freeSpaceListDescriptor != null)
			{
				Output(" - Free list descr");

//ORIGINAL LINE: foreach (uint nBlock in freeSpaceListDescriptor)
				for (int nBlock : freeSpaceListDescriptor)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}

			if (freeList != null)
			{
				Output(" - Free list");

//ORIGINAL LINE: foreach (uint nBlock in freeList)
				for (int nBlock : freeList)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}

			if (lastShotListDescriptor != null)
			{
				Output(" - last snap descriptor");

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: foreach (uint nBlock in lastShotListDescriptor)
				for (int nBlock : lastShotListDescriptor)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}

			if (lastShotContentList != null)
			{
				Output(" - last snap blocks");

//ORIGINAL LINE: foreach (uint nBlock in lastShotContentList)
				for (int nBlock : lastShotContentList)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}

			if (prevShotListDescriptor != null)
			{
				Output(" - Prev snap descr");

				for (int nBlock : prevShotListDescriptor)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}

			if (prevShotContentList != null)
			{
				Output(" - Prev snap blocks");

				for (int nBlock : prevShotContentList)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}
			OutputLine("Done");

			{
				OutputLine("");
				OutputLine("Looking for unreferenced blocks between disk_start_page and free_start");
				OutputLine("-----------------");
				//List<uint> g = new List<uint>();
				int count = 0;

				for (int nBlock = superBlock.getDisk_start_page(); nBlock < superBlock.getFree_start(); nBlock++)
				{
					if (!t.containsKey(nBlock))
					{
						//g.Add(nBlock);
						count++;
					}
				}

				if (count > 0)
				{
					OutputLine("  lost blocks " + (new Integer(count)).toString() + ".");
				}

				OutputLine("Done");
			}

			{
				OutputLine("");
				OutputLine("Looking for references to blocks out of disk_start_page-disk_page_count");
				OutputLine("-----------------");
				//List<uint> g = new List<uint>();
				int count = 0;

				int leftBorder = superBlock.getDisk_start_page();

				int rightBorder = superBlock.getDisk_page_count();

				for (int nBlock : t.keySet())
				{
					if (nBlock < leftBorder || nBlock >= rightBorder)
					{
						count++;
					}
				}

				if (count > 0)
				{
					OutputLine("  out of bounds block references " + (new Integer(count)).toString() + " ");
				}
				else
				{
					OutputLine("None");
				}

				OutputLine("Done");
			}

		}

		magicsStats();
		
		OutputLine("");
		OutputLine("FS check finished " + new java.util.Date().toString() + ".");
	}

	/** 
	 * Magics statistics - how many blocks of given magic type exist. Inexact. :(
	 **/
	private void magicsStats() {
		{
			// есть вероятность ошибок в большую сторону, так как блоки данных могут быть приняты за блоки с меджиками
			OutputLine("");
			OutputLine("Block type statistics (inexact, tends to give higher numbers)");
			OutputLine("-----------------");

			//ORIGINAL LINE: Dictionary<Magics, uint> magicsStatistics = new Dictionary<Magics, uint>();
			java.util.HashMap<Magics, Integer> magicsStatistics = new java.util.HashMap<Magics, Integer>();

			for( Magics enumVal : Magics.values() )
			{
				magicsStatistics.put(enumVal, 0);
			}

			//int last_block = superBlock.getDisk_start_page() + superBlock.getDisk_page_count() - 1;
			int last_block = superBlock.getDisk_page_count();

			for (int nBlock = superBlock.getDisk_start_page(); nBlock < last_block; nBlock++)
			{
				Block block = m_Image.ReadBlock(nBlock);

				if (magicsStatistics.containsKey(block.getMagic()))
				{
					//magicsStatistics.get(block.getMagic())++;
					Integer integer = magicsStatistics.get(block.getMagic());
					integer = integer+1;
				}
			}

			for (Magics magic : magicsStatistics.keySet())
			{
				OutputLine(magic.toString() + " " + magicsStatistics.get(magic).toString());
			}

			OutputLine("Done");
		}
	}

	public final void blockTypeCheck(ListDescriptor descriptor, Magics requestedType)
	{
		for (int nBlock : descriptor)
		{
			Block block = m_Image.ReadBlock(nBlock);

			if (block.getMagic() != requestedType)
			{
				OutputLine("Expected block type " + requestedType.toString() + ", found - " + block.getMagic());
			}
		}
	}

	public final void Output(String message)
	{
		System.out.println(message);
	}

	public final void OutputLine(String message)
	{
		Output(message+ "\n");
	}

	
	public final void Output(BlockList block_list)
	{
		OutputLine("  blocks total " + block_list.size());
		OutputLine("  null blocks " + (new Integer(block_list.getNullCount())).toString());
	}

	public final void Output(ListDescriptor descriptor)
	{
		OutputLine("  blocks used by descriptor " + descriptor.size());
	}

}