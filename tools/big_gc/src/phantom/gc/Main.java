/**
 * 
 */
package phantom.gc;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;

import phantom.data.DataLoadException;
import phantom.data.ObjectHeader;

/**
 * @author dz
 *
 */
public class Main {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		if( args.length > 1)
		{
			System.out.println("gc <phantom_memory_dump_file>");
			return;
		}

		String fn = "../../run/last_snap.data";
		
		if( args.length == 1 )
			fn = args[0];
		
		
		System.out.println("Processing "+fn);
		
		RandomAccessFile aFile;
		try {
			aFile = new RandomAccessFile(fn, "r");
	        FileChannel inChannel = aFile.getChannel();
	        MappedByteBuffer buffer1 = inChannel.map(FileChannel.MapMode.READ_ONLY, 0, inChannel.size());
	        
	        ByteBuffer buffer = buffer1.slice();
	        

	        buffer.rewind();
	        //buffer.load();

	        // NB! Seems to be broken? Reads 0x7f550000 instead of 0x7FAA7F55
	        buffer.order(ByteOrder.LITTLE_ENDIAN);
	        
	        while( buffer.position() < inChannel.size() ) {
				ObjectHeader h = new ObjectHeader();
				
				try {
					h.loadHeader(buffer);
				} catch (DataLoadException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
					break;
				}
				
			}
	        
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}

}
