package ru.dz.pfsck;


public class Phantom_Disk_Superblock extends Block
{
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("Версия")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public System.UInt32 getVersion()
	public final int getVersion()
	{
//C# TO JAVA CONVERTER TODO TASK: There is no Java equivalent to 'sizeof':
		return BitConverter.ToUInt32(map, 4);
	}

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getChecksum()
	public final int getChecksum()
	{
		return BitConverter.ToUInt32(map, 8);
	}

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getBlocksize()
	public final int getBlocksize()
	{
		return BitConverter.ToUInt32(map, 12);
	}

//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("address of 2nd copy or 0 if no.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getSb2_addr()
	public final int getSb2_addr()
	{
		return BitConverter.ToUInt32(map, 16);
	}

//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("address of 3nd copy or 0 if no.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getSb3_addr()
	public final int getSb3_addr()
	{
		return BitConverter.ToUInt32(map, 20);
	}

	/** 
	 num of 1st page we can access
	 
	*/
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("num of 1st page we can access")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getDisk_start_page()
	public final int getDisk_start_page()
	{
		return BitConverter.ToUInt32(map, 24);
	}

	/** 
	 num of 1st unavail page.
	 
	*/
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("num of 1st unavail page.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getDisk_page_count()
	public final int getDisk_page_count()
	{
		return BitConverter.ToUInt32(map, 28);
	}

	/** 
	 number of the first block that was never allocated. This and following blocks are not in any list and are free.
	 
	*/
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("number of the first block that was never allocated. This and following blocks are not in any list and are free.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getFree_start()
	public final int getFree_start()
	{
		return BitConverter.ToUInt32(map, 32);
	}

	/** 
	 free list head or 0 if no.
	 
	*/
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("free list head or 0 if no.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getFree_list()
	public final int getFree_list()
	{
		return BitConverter.ToUInt32(map, 36);
	}

	/** 
	 the latest snapshot or 0 if no.
	 
	*/
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("the latest snapshot or 0 if no.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getLast_snap()
	public final int getLast_snap()
	{
		return BitConverter.ToUInt32(map, 44);
	}

	/** 
	 previous snapshot or 0 if no.
	 
	*/
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("previous snapshot or 0 if no.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getPrev_snap()
	public final int getPrev_snap()
	{
		return BitConverter.ToUInt32(map, 64);
	}

	/** 
	 32 bits  - DISK_STRUCT_MAGIC_SUPER_2
	 
	*/
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("32 bits  - DISK_STRUCT_MAGIC_SUPER_2")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getMagic2()
	public final int getMagic2()
	{
		return BitConverter.ToUInt32(map, 84);
	}

	/** 
	 List of blocks with bootloader image or 0.
	 
	*/
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("List of blocks with bootloader image or 0.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getBoot_list()
	public final int getBoot_list()
	{
		return BitConverter.ToUInt32(map, 88);
	}

	/** 
	 List of blocks with kernel image or 0.
	 
	*/
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("List of blocks with kernel image or 0.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getKernel_list()
	public final int getKernel_list()
	{
		return BitConverter.ToUInt32(map, 92);
	}

	//disk_page_no_t disk_start_page; // num of 1st page we can access
	//disk_page_no_t disk_page_count; // num of 1st unavail page.

	//disk_page_no_t free_start;	// number of the first block that was never allocated. This and following blocks are not in any list and are free.
	//disk_page_no_t free_list;	// free list head or 0 if no.

	//disk_page_no_t boot_list;			// List of blocks with bootloader image or 0.
	//disk_page_no_t kernel_list;		// List of blocks with kernel image or 0.
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: System.UInt32[] boot_module = new System.UInt32[ConstantProvider.DISK_STRUCT_N_MODULES];
	private int[] boot_module = new int[ConstantProvider.DISK_STRUCT_N_MODULES]; // List of blocks for boot time modules, up to 32.

	//	    char_t[]    sys_name = new char_t[DISK_STRUCT_SB_SYSNAME_SIZE];	// 0-terminated description for bootloader menu

	private long object_space_address; // Object space expects to be loaded here

	public Phantom_Disk_Superblock(Block b)
	{
		map = b.map.duplicate();
	}

//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("Правильно ли рассчитана контрольная сумма записанная в «суперблоке»")]
	public final boolean getIs_checksum_correct()
	{
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: uint count = Blocksize;
		int count = getBlocksize();
		int sum = 0;
		while (count-- != 0)
		{
			sum += map.get(count);
		}

		if (sum == 0)
		{
			return true;
		}

		return false;
	}
	
	@Override
	public String toString()
	{
	    return
	        "version: " + getVersion()
	        + "\r\nchecksum: " + getChecksum()
	        + "\r\nsb2_addr: " + getSb2_addr()
	        + "\r\nsb3_addr: " + getSb3_addr()
	        + "\r\ndisk_start_page: " + getDisk_start_page()
	        + "\r\ndisk_page_count: " + getDisk_page_count()
	        + "\r\nfree_start: " + getFree_start()
	        + "\r\nfree_list: " + getFree_list()
	        + "\r\nboot_list: " + getBoot_list()
	        + "\r\nkernel_list: " + getKernel_list();
	}
}