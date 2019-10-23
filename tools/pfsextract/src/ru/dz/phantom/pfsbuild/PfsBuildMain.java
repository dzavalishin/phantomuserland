package ru.dz.phantom.pfsbuild;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

import ru.dz.phantom.disk.Const;
import ru.dz.phantom.disk.PhantomSuperBlock;

public class PfsBuildMain {
	static long sbaddr[] = Const.DISK_STRUCT_SB_OFFSET_LIST;
	final static int disk_size = 1024*1024*500; // TODO 500 mb hardcode

	public static void main(String[] args) throws IOException 
	{
		System.out.println("Phantom disk builder");
		if( (args.length < 1) || (args.length > 2)) {
			System.out.println("\nUsage: pfsbuild phantom_disk_file_name [snapshot_image]");
			System.out.println("\nWill construct Phantom disk with given memory image inside");
			System.out.println("or empty, if image file is not given.");
			return;
		}

		// Open Phantom disk image file
		File imgfn = new File(args[0]);
		RandomAccessFile img = new RandomAccessFile(imgfn, "rw");
		FileChannel fc = img.getChannel();
	
		RandomAccessFile mem = null;
		
		if(args.length == 2)
		{
			File memfn = new File(args[0]);
			mem = new RandomAccessFile(memfn, "r");
		}
	
		// Prepare Phantom superblock
		PhantomSuperBlock phantomSuperBlock = new PhantomSuperBlock(disk_size, "Unnamed Phantom system");
		
		// We do it in a simple way. Use space after last superblock copy in a linear manner, 
		// then update FreeStart - position of a first free block.
		
		// Copy mem to img
		if(mem != null)
		{
			// NB: last block must be really last block used by storeFileToImage()
			// and first block of pagelist for snapshot
			int lastBlock = storeFileToImage( img, mem, (int)sbaddr[3]+1 ); // Start pos 
			phantomSuperBlock.updateLastSnap(lastBlock);
			phantomSuperBlock.updateFreeStart(lastBlock+1);
		}
		
		// Write Phantom superblock
		ByteBuffer sb = phantomSuperBlock.getSB();
		writeSb(fc, sb);
		img.setLength((long)(disk_size * sb.getInt(Const.blocksize)));
		
		fc.close();
		img.close();
		
		
	}

	private static int storeFileToImage(RandomAccessFile img, RandomAccessFile mem, int blk ) throws IOException 
	{
		int curr = blk;
		int data_start_block = curr;
		
		img.seek(blk * Const.DISK_STRUCT_BS);
		
		while(true)
		{
			byte [] buf = new byte[Const.DISK_STRUCT_BS];
			
			int nread = mem.read(buf);
			
			if( nread != Const.DISK_STRUCT_BS)
			{
				System.err.println("mem image size incorrect");
				break;
			}
			
			if(nread < 0) break;
			
			img.write(buf);
		}

		int data_end_block = curr;

		// now write pagelist starting from the end 
		
		// TODO
		
		return 0;
	}

	public static void writeSb(FileChannel fc, ByteBuffer sb) throws IOException {

		System.out.println("Disk size is " + sb.getInt(Const.disk_page_count) + " pages");
		System.out.println("Disk block size is " + sb.getInt(Const.blocksize));
		
		// Write superblock to img
		long sba = sbaddr[0] * sb.getInt(Const.blocksize);
		System.out.format("Writing superblock to 0x%x %n", sba);
		
		//img.seek(sba);
		
		fc.position(sba);		
		fc.write(sb);
	}

}
