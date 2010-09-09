package ru.dz.pfsck;


public class ConstantProvider
{
	public static final int DISK_STRUCT_N_MODULES = 30;
	/** 
	 длина блока
	 
	*/
	public static final int DISK_STRUCT_BS = 4096;
	public static final int PHANTOM_DEFAULT_DISK_START = 0x10;

	public enum DISK_STRUCT_SB_OFFSETS //DISK_STRUCT_SB_OFFSET_LIST
	{
		First(0x10),
		Second(0x100),
		Third(0x220),
		Fourth(0x333);

		private int intValue;
		private static java.util.HashMap<Integer, DISK_STRUCT_SB_OFFSETS> mappings;
		private synchronized static java.util.HashMap<Integer, DISK_STRUCT_SB_OFFSETS> getMappings()
		{
			if (mappings == null)
			{
				mappings = new java.util.HashMap<Integer, DISK_STRUCT_SB_OFFSETS>();
			}
			return mappings;
		}

		private DISK_STRUCT_SB_OFFSETS(int value)
		{
			intValue = value;
			DISK_STRUCT_SB_OFFSETS.getMappings().put(value, this);
		}

		public int getValue()
		{
			return intValue;
		}

		public static DISK_STRUCT_SB_OFFSETS forValue(int value)
		{
			return getMappings().get(value);
		}
	}
}
