package ru.dz.phantom.pfsextract;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
//import ru.dz.phantom.disk.PhantomSuperBlock;
import ru.dz.phantom.disk.Const;

/*
* Phantom OS
*
* Phantom 'FS' extraction code - to debug snapshotting.
*
* @converter_from_C_to_Java: oleg.kakaulin
*
**/

class Main {

	static long sbaddr[] = Const.DISK_STRUCT_SB_OFFSET_LIST;

	/**
	 * @param args
	 */
	public static void main(String[] args) throws IOException {
		// TODO Auto-generated method stub
		System.out.println("Phantom disk extractor");
		if (args.length != 1) {
			System.out.println("\nUsage: pfsextract phantom_disk_file_name");
			return;
		}
		
		// Open Phantom disk image file
		File imgfn = new File(args[0]);
		RandomAccessFile img = new RandomAccessFile(imgfn, "r");
		
		// Try read and check one of four superblock
		ByteBuffer sb = ByteBuffer.allocate(Const.DISK_STRUCT_BS);
		FileChannel fc = img.getChannel();
		
		int magic, magic2, 
			rsb_success = 0;
		for (int i = 0; i < 4; i++) {
			fc.position(sbaddr[i] * Const.DISK_STRUCT_BS);
			fc.read(sb);
			sb.order(ByteOrder.LITTLE_ENDIAN);
			magic = sb.getInt(Const.magic);
			magic2 = sb.getInt(Const.magic2);
			if ((magic == Const.DISK_STRUCT_MAGIC_SUPERBLOCK) && 
					(magic2 == Const.DISK_STRUCT_MAGIC_SUPER_2)) {
				rsb_success = 1;
				break;
			}
		}

		if (rsb_success == 0) {
			System.err.println("pfsextract: Wrong superblock's magics\n");
			fc.close();
			img.close();
			return;
		}
		
		// Display some important system information
		int version = sb.getInt(Const.version);
		System.out.format("FS Ver. %d.%d, osname '%s'%n", version >> 16, version & 0xffff, system_name(sb));
		System.out.format("Free list at %d%n", sb.getInt(Const.free_list));
		System.out.format("Disk is %d blocks, untouched space from %d\n", sb.getInt(Const.disk_page_count), sb.getInt(Const.free_start));
		
		// Upload some Phantom disk system areas:
		//	- the latest snapshot
		storeToFile(fc, sb.getInt(Const.last_snap), "last_snap.data", Const.DISK_STRUCT_MAGIC_SNAP_LIST);
		//	- previous snapshot
		storeToFile(fc, sb.getInt(Const.prev_snap), "prev_snap.data", Const.DISK_STRUCT_MAGIC_SNAP_LIST);
		//	- list of blocks with bootloader image
		storeToFile(fc, sb.getInt(Const.boot_list), "boot.data", Const.DISK_STRUCT_MAGIC_BOOT_LOADER);
		//	- list of blocks with kernel image
		storeToFile(fc, sb.getInt(Const.kernel_list), "kernel.data", Const.DISK_STRUCT_MAGIC_BOOT_KERNEL);
		//	- list of blocks for boot time modules
		storeToFile(fc, sb.getInt(Const.boot_module), "boot_module.0.data", Const.DISK_STRUCT_MAGIC_BOOT_MODULE);
		storeToFile(fc, sb.getInt(Const.boot_module + 4), "boot_module.1.data", Const.DISK_STRUCT_MAGIC_BOOT_MODULE);
		storeToFile(fc, sb.getInt(Const.boot_module + 8), "boot_module.2.data", Const.DISK_STRUCT_MAGIC_BOOT_MODULE);
		
		fc.close();
		img.close();
	}
	
	static String system_name(ByteBuffer sb) {
		byte b[] = new byte[Const.DISK_STRUCT_SB_SYSNAME_SIZE];
		sb.position(Const.sys_name);
		sb.get(b, 0, Const.DISK_STRUCT_SB_SYSNAME_SIZE);
		String name = new String(b);
		
		return name.trim();		
	}
	
	static void getBootModuleInfo(ByteBuffer modBlock, int magic ) {
		System.out.println("getBootModuleInfo");
		if (modBlock.getInt(0) == magic) {			
			byte b[] = new byte[Const.DISK_STRUCT_BM_NAME_SIZE];
			modBlock.position(16);
			modBlock.get(b, 0, Const.DISK_STRUCT_BM_NAME_SIZE);
			String bm_name = new String(b);
			System.out.format("boot module name: %s%n", bm_name.trim());
		}
	}
	
	static void storeToFile(FileChannel ifc, int startBlk, String outFn, int _magic ) throws IOException {
		if (startBlk == 0) return;  // no data
		// Read the head block 
		ByteBuffer headBlock = ByteBuffer.allocate(Const.DISK_STRUCT_BS);
		ifc.position(startBlk * Const.DISK_STRUCT_BS);
		ifc.read(headBlock);
		headBlock.order(ByteOrder.LITTLE_ENDIAN);
		
		// Do we need boot module information?
		if (_magic == Const.DISK_STRUCT_MAGIC_BOOT_MODULE) {
			getBootModuleInfo(headBlock, _magic);
			return;
		}

		// Open/create output file
		File outf = new File(outFn);
		RandomAccessFile out = new RandomAccessFile(outf, "rws");
		FileChannel ofc = out.getChannel();
		
		// Scan all headBlocks
		ByteBuffer dataBlock = ByteBuffer.allocate(Const.DISK_STRUCT_BS);
		do {
			int magic = headBlock.getInt(0); // headBlock.head.magic
			int used = headBlock.getInt(4); // headBlock.head.used
			// Scan blocklist in headBlock
			if (magic != _magic) {
				System.out.format("pfsextract: bad magic %X instead of %X%n", magic, _magic);
				break;
			}
			if (used > 0) {
				for (int i = 0; i < used; i++) {
					int nextDataNum = headBlock.getInt(16 + (i * 4));
					//if (nextDataNum == 0) break;
					ifc.position(nextDataNum * Const.DISK_STRUCT_BS);
					dataBlock.position(0);
					ifc.read(dataBlock);
					dataBlock.position(0);
					ofc.write(dataBlock);
				}
			}
			// go to next blocklist page
			int nextPage = headBlock.getInt(8); // headBlock.head.next
			if (nextPage == 0) break; 
			ifc.position(nextPage * Const.DISK_STRUCT_BS);
			headBlock.position(0);
			ifc.read(headBlock);
			headBlock.order(ByteOrder.LITTLE_ENDIAN);
		} while (true); 
		
		ofc.close();
		out.close();
	}

}
