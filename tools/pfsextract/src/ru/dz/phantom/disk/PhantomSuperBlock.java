package ru.dz.phantom.disk;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import ru.dz.phantom.disk.Const;

public class PhantomSuperBlock {
	
	ByteBuffer map;
	
	public PhantomSuperBlock(int n_pages, String title)
	{
		// Fill superblock with basic formatting info.
		map = ByteBuffer.allocate(Const.DISK_STRUCT_BS);
		map.order(ByteOrder.LITTLE_ENDIAN);
		map.putInt(Const.magic, Const.DISK_STRUCT_MAGIC_SUPERBLOCK);
		map.putInt(Const.version, Const.DISK_STRUCT_VERSION);
		map.putInt(Const.blocksize, Const.DISK_STRUCT_BS);
		map.putInt(Const.sb2_addr, 0);
		map.putInt(Const.sb3_addr, 0);
		map.putInt(Const.disk_start_page, Const.PHANTOM_DEFAULT_DISK_START);
		map.putInt(Const.disk_page_count, n_pages);
		map.putInt(Const.free_start, 1);
		map.putInt(Const.free_list, 0);
		map.putInt(Const.magic2, Const.DISK_STRUCT_MAGIC_SUPER_2);
		map.put(Const.fs_is_clean, (byte)0xff);
		int i = 0;
		while ((i < Const.DISK_STRUCT_SB_SYSNAME_SIZE) && (i < title.length())) {
			map.put(Const.sys_name+i, (byte)title.charAt(i));
			i++;
		}
		map.putInt(Const.checksum, calcSBchecksum(map));
		
	}
	
	int calcSBchecksum(ByteBuffer sb)
	{
		map.putInt(Const.checksum, 0);
		int counter = 0;
		int i = 0;
		while (i < map.limit()) {
			counter += (int)(map.get(i) & 0xff);
			i++;
		}
		return counter;
	}
	
	public ByteBuffer getSB()
	{
		return map;
	}

}
