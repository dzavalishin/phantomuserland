/**
 * 
 */
package phantom.gc;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;

import phantom.data.DataLoadException;
import phantom.data.ObjectHeader;

/**
 * @author dz
 *
 */
public class Main {

	// On Intel persistent virtual address space begins here
	private static final long OBJECT_VMEM_SHIFT = 0x80000000L;

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

	        System.out.println("mem size = "+inChannel.size()/1024+"K");
	        //System.out.println("int = "+Integer.toHexString(aFile.readInt()) );

	        //MappedByteBuffer buffer;
	        ByteBuffer buffer; 
	        
	        buffer = inChannel.map(FileChannel.MapMode.READ_ONLY, 0, inChannel.size());

	        buffer.order(ByteOrder.LITTLE_ENDIAN);

	        /*
	        for(int i = 0; i <10; i++)
	        {
	        	//byte b = buffer.get();
	        	//System.out.print(" "+Integer.toHexString( ((int)b) & 0xFF ) );

	        	int v = buffer.getInt();
	        	System.out.print(" "+Integer.toHexString( v ) );
	        }
	        System.out.println();
			*/
	        buffer.rewind();
	        //buffer.load();

	        // NB! Seems to be broken? Reads 0x7f550000 instead of 0x7FAA7F55
	        //buffer.order(ByteOrder.LITTLE_ENDIAN);
	        
	        while( buffer.position() < inChannel.size() ) 
	        {
	        	long objectAddress = buffer.position()+OBJECT_VMEM_SHIFT;
	        	
				ObjectHeader h = new ObjectHeader();
				try {
					h.loadHeader(buffer);
				} catch (DataLoadException e) {
					System.out.println("pos 0x"+Integer.toHexString( buffer.position() ) );
					e.printStackTrace();
					break;
				}
				
				System.out.print(String.format("%08x: ", objectAddress));
				h.dump();
				
				// Here we have object (or free space) information loaded in h, 
				// it's address in Phantom is objectAddress
				
				// store it to hash map
			}
	        
	        // Actually we come here on "phantom.data.DataLoadException: object header marker is wrong"
	        // for we can't really tell exact size of Phantom persistent memory. Anyway, lets do GC
	        
	        // process hash map and do GC
	        
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}

}
