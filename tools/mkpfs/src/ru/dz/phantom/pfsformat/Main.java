package ru.dz.phantom.pfsformat;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import ru.dz.phantom.disk.PhantomSuperBlock;
import ru.dz.phantom.disk.Const;

/**
 * Phantom 'FS' format code - to prepare image files, mostly.
 */


/**
 * @converter_from_C_to_Java: oleg.kakaulin
 *
 */
class Main {
	
	final static int disk_size = 1024*20;
	static long sbaddr[] = Const.DISK_STRUCT_SB_OFFSET_LIST;

	/**
	 * @param args
	 */
	public static void main(String[] args) throws IOException {
		// TODO Auto-generated method stub
		System.out.println("Phantom disk formatter");
		if (args.length != 1) {
			System.out.println("\nUsage: pfsformat phantom_disk_file_name");
			return;
		}
		
		// Open/Create Phantom disk image file
		File imgfn = new File(args[0]);
		RandomAccessFile img = new RandomAccessFile(imgfn, "rw");
		// Prepare Phantom superblock
		ByteBuffer sb = new PhantomSuperBlock(disk_size, "Unnamed Phantom system").getSB();

		System.out.println("Disk size is " + sb.getInt(Const.disk_page_count) + " pages");
		System.out.println("Disk block size is " + sb.getInt(Const.blocksize));
		
		// Write superblock to img
		long sba = sbaddr[0] * sb.getInt(Const.blocksize);
		img.setLength((long)(disk_size * sb.getInt(Const.blocksize)));
		System.out.format("Writing superblock to 0x%x %n", sba);
		img.seek(sba);
		FileChannel fc = img.getChannel();
		fc.write(sb);
		
		fc.close();
		img.close();
	}
}

	
