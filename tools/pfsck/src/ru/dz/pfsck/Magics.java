package ru.dz.pfsck;

public enum Magics 
{
	DISK_STRUCT_MAGIC_SUPERBLOCK(0xC001AC1D),
	DISK_STRUCT_MAGIC_SUPER_2(0xB0B0ADAD),
	DISK_STRUCT_MAGIC_FREEHEAD(0xC001FBFB),
	DISK_STRUCT_MAGIC_CONST_LIST(0xC001CBCB),
	DISK_STRUCT_MAGIC_SNAP_LIST(0xC001C0C0),
	DISK_STRUCT_MAGIC_BAD_LIST(0xC001BAD0),
	DISK_STRUCT_MAGIC_LOG_LIST(0xC001100C),
	DISK_STRUCT_MAGIC_PROGRESS_PAGE(0xDADADADA),
	DISK_STRUCT_MAGIC_BOOT_MODULE(0xC001B001),
	DISK_STRUCT_MAGIC_BOOT_LOADER(0xB001B001),
	DISK_STRUCT_MAGIC_BOOT_KERNEL(0xB001AC1D);

	private int intValue;
	private static java.util.HashMap<Integer, Magics> mappings;
	private synchronized static java.util.HashMap<Integer, Magics> getMappings()
	{
		if (mappings == null)
		{
			mappings = new java.util.HashMap<Integer, Magics>();
		}
		return mappings;
	}

	private Magics(int value)
	{
		intValue = value;
		Magics.getMappings().put(value, this);
	}

	public int getValue()
	{
		return intValue;
	}

	public static Magics forValue(int value)
	{
		return getMappings().get(value);
	}
}