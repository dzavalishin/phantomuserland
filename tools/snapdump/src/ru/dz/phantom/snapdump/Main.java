package ru.dz.phantom.snapdump;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteOrder;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;

import sun.nio.ch.FileChannelImpl;

public class Main {

	/**
	 * @param args
	 * @throws IOException 
	 */
	public static void main(String[] args) throws IOException {
		
		File snapfn = new File(args[0]);
		System.out.println("SnapDump: "+snapfn);

		RandomAccessFile snap = new RandomAccessFile(snapfn,"r");
		
		FileChannel fc = snap.getChannel();
		
		MappedByteBuffer map = fc.map(MapMode.READ_ONLY, 0, fc.size());
		map.order(ByteOrder.LITTLE_ENDIAN);
		
		Visitor v = new Visitor(map);
		
		v.go();
		
		VisitedMap visitedMap = v.getMap(); // all visited objects
		
		Scanner s = new Scanner(map);
			
		try {
			s.scan(visitedMap);
		} catch (InvalidPhantomMemoryException e) {
			e.printStackTrace();
		}
		
		s.printStats();
	}

}
