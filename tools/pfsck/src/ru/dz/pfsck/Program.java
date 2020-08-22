package ru.dz.pfsck;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteOrder;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;

public final class Program
{

	public static final ByteOrder BYTE_ORDER = ByteOrder.LITTLE_ENDIAN;
	private static int errors = 0;
	private static int fatalErrors = 0;
	//public static final ByteOrder BYTE_ORDER = ByteOrder.BIG_ENDIAN;

	/**
	 * @param args
	 * @throws IOException 
	 */
	public static void main(String[] args) throws IOException {
		
		File snapfn = new File(args[0]);
		System.out.println("pfsck: "+snapfn);
		
		RandomAccessFile snap = new RandomAccessFile(snapfn,"r");
		
		FileChannel fc = snap.getChannel();
		
		MappedByteBuffer map = fc.map(MapMode.READ_ONLY, 0, fc.size());
		map.order(BYTE_ORDER);
		
		
		FsChecker form = new FsChecker(map);
		form.Check();

		fc.close();

		System.out.println("Finished, "+errors+" errors");
		
		System.exit( errors == 0 ? 0 : 1);
	}

	public static void reportError(String string) {
		System.out.println("Error: "+string);
		errors++;
		
	}

	public static void reportFatalError(String string) {
		System.out.println("Fatal error: "+string);
		errors++;
		fatalErrors ++;
		
	}


	public static boolean isVerbose() {
		return false;
	}


}