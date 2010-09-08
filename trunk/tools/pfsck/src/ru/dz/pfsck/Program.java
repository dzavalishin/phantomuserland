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
		
		
		Form1 form = new Form1(map);
		form.Check();
	}
}