package ru.dz.pfsck;


public class Phantom_Disk_Superblock extends Block
{
	public final int getVersion()
	{
		return BitConverter.ToUInt32(map, 4);
	}

	public int getVersionMajor() { return getVersion() >> 16; }
	public int getVersionMinor() { return getVersion() & 0xFFFF; }
	
	public final int getChecksum()
	{
		return BitConverter.ToUInt32(map, 8);
	}

	public final int getBlocksize()
	{
		return BitConverter.ToUInt32(map, 12);
	}

	public final int getSb2_addr()
	{
		return BitConverter.ToUInt32(map, 16);
	}

	public final int getSb3_addr()
	{
		return BitConverter.ToUInt32(map, 20);
	}

	/** 
	 * number of 1st page we can access	 
	**/
	public final int getDisk_start_page()
	{
		return BitConverter.ToUInt32(map, 24);
	}

	/** 
	 num of 1st unavail page.	 
	*/
	public final int getDisk_page_count()
	{
		return BitConverter.ToUInt32(map, 28);
	}

	/** 
	 number of the first block that was never allocated. This and following blocks are not in any list and are free.	 
	*/
	public final int getFree_start()
	{
		return BitConverter.ToUInt32(map, 32);
	}

	/** 
	 free list head or 0 if no. 
	*/
	public final int getFree_list()
	{
		return BitConverter.ToUInt32(map, 36);
	}

	/** is FS clean */
	public final boolean isClean()
	{
		return map.get(40) != 0; // 3 more unused chars follow
	}
	
	
	/** 
	 the latest snapshot or 0 if no.	 
	*/
	public final int getLast_snap()
	{
		return BitConverter.ToUInt32(map, 44);
	}

	/** 
	 previous snapshot or 0 if no.	 
	*/
	public final int getPrev_snap()
	{
		return BitConverter.ToUInt32(map, 64);
	}

	/** 
	 32 bits  - DISK_STRUCT_MAGIC_SUPER_2	 
	*/
	public final int getMagic2()
	{
		return BitConverter.ToUInt32(map, 84);
	}

	/** 
	 List of blocks with bootloader image or 0.	 
	*/
	public final int getBoot_list()
	{
		return BitConverter.ToUInt32(map, 88);
	}

	/** 
	 List of blocks with kernel image or 0.	 
	*/
	public final int getKernel_list()
	{
		return BitConverter.ToUInt32(map, 92);
	}

	private int[] boot_module = new int[ConstantProvider.DISK_STRUCT_N_MODULES]; // List of blocks for boot time modules, up to 32.


	private long object_space_address; // Object space expects to be loaded here

	public Phantom_Disk_Superblock(Block b)
	{
		map = b.map.duplicate();
		map.order(Program.BYTE_ORDER);
	}

	public final boolean getIs_checksum_correct()
	{
		/*
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: uint count = Blocksize;
		int count = getBlocksize();
		int sum = 0;
		while (count-- != 0)
		{
			sum += map.get(count);
		}

		return sum == 0;
		*/
		
		return true;
	}
	
	@Override
	public String toString()
	{
	    return
	        //"version: 0x" + String.format("%08X", getVersion())
	        "version: " + getVersionMajor() + "." + getVersionMinor()
	        + "\nchecksum: " + getChecksum()
	        + "\nsb2_addr: " + getSb2_addr()
	        + "\nsb3_addr: " + getSb3_addr()
	        + "\ndisk_start_page: " + getDisk_start_page()
	        + "\ndisk_page_count: " + getDisk_page_count()
	        + "\nfree_start: " + getFree_start()
	        + "\nfree_list: " + getFree_list()

	        + "\nold snap:  " + ((getPrev_snap() != 0 ? (new Integer(getPrev_snap())).toString() : "none")) 
			+ "\nthis snap: " + ((getLast_snap() != 0 ? (new Integer(getLast_snap())).toString() : "none"))
	        
	        + "\nboot_list: " + getBoot_list()
	        + "\nkernel_list: " + getKernel_list()
	        
	        + "\n\nis clean: " + isClean()
	        +"\n"
	        ;
	}
}