public class Phantom_Disk_Superblock extends Block
{
//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("Версия")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public System.UInt32 getVersion()
	public final int getVersion()
	{
//C# TO JAVA CONVERTER TODO TASK: There is no Java equivalent to 'sizeof':
		return BitConverter.ToUInt32(m_Buffer, sizeof(int));
	}

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getChecksum()
	public final int getChecksum()
	{
		return BitConverter.ToUInt32(m_Buffer, 8);
	}

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getBlocksize()
	public final int getBlocksize()
	{
		return BitConverter.ToUInt32(m_Buffer, 12);
	}

//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("address of 2nd copy or 0 if no.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getSb2_addr()
	public final int getSb2_addr()
	{
		return BitConverter.ToUInt32(m_Buffer, 16);
	}

//C# TO JAVA CONVERTER TODO TASK: Java annotations will not correspond to .NET attributes:
	//[DescriptionAttribute("address of 3nd copy or 0 if no.")]
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public UInt32 getSb3_addr()
	public final int getSb3_addr()
	{
		return BitConverter.ToUInt32(m_Buffer, 20);
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
		return BitConverter.ToUInt32(m_Buffer, 24);
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
		return BitConverter.ToUInt32(m_Buffer, 28);
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
		return BitConverter.ToUInt32(m_Buffer, 32);
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
		return BitConverter.ToUInt32(m_Buffer, 36);
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
		return BitConverter.ToUInt32(m_Buffer, 44);
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
		return BitConverter.ToUInt32(m_Buffer, 64);
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
		return BitConverter.ToUInt32(m_Buffer, 84);
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
		return BitConverter.ToUInt32(m_Buffer, 88);
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
		return BitConverter.ToUInt32(m_Buffer, 92);
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

	public Phantom_Disk_Superblock(byte[] buffer)
	{
		m_Buffer = buffer;
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
			sum += m_Buffer[count];
		}

		if (sum == 0)
		{
			return true;
		}

		return false;
	}
	//public override string ToString()
	//{
	//    return
	//        "version: " + Version.ToString()
	//        + "\r\nchecksum: " + Checksum.ToString()
	//        + "\r\nsb2_addr: " + Sb2_addr.ToString()
	//        + "\r\nsb3_addr: " + Sb3_addr.ToString()
	//        + "\r\ndisk_start_page: " + Disk_start_page.ToString()
	//        + "\r\ndisk_page_count: " + Disk_page_count.ToString()
	//        + "\r\nfree_start: " + Free_start.ToString()
	//        + "\r\nfree_list: " + Free_list.ToString()
	//        + "\r\nboot_list: " + Boot_list.ToString()
	//        + "\r\nkernel_list: " + Kernel_list.ToString();
	//}
}