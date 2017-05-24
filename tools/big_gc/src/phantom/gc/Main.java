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
import java.util.HashMap;
import java.util.Map;

import phantom.data.DataLoadException;
import phantom.data.ObjectHeader;

/**
 * @author dz
 *
 */
public class Main {

	// On Intel persistent virtual address space begins here
	private static final long OBJECT_VMEM_SHIFT = 0x80000000L;
	
	// Later we will have arenas info right in the virtual memory and will take sizes from there
	private static final long OBJECT_VMEM_SIZE = 0x2000000L; // TODO ERROR size hardcode

	// Provide exit code to be used in shell scripts
	private static final int EXIT_CODE_OK = 0;		// Checked memory and found it to be correct
	private static final int EXIT_CODE_FAILED = 1;	// Memory contents are broken
	private static final int EXIT_CODE_ERROR = 2;	// Unable to load all the memory map

	static Map<Long,ObjectHeader> objects = new HashMap<Long,ObjectHeader>();
	static long allocatedSize = 0;
	static long freeSize = 0;
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		if( args.length > 1)
		{
			System.out.println("gc <phantom_memory_dump_file>");
			System.exit(EXIT_CODE_ERROR);
		}

		String fn = "../../run/last_snap.data";
		
		if( args.length == 1 )
			fn = args[0];
		
		
		System.out.println("Loding "+fn);
		
		try {
			process(fn);	        
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}

	private static void process(String fn) throws FileNotFoundException, IOException {
		RandomAccessFile aFile;
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
		
		while( (buffer.position() < inChannel.size()) && (buffer.position() < OBJECT_VMEM_SIZE) ) 
		{
			long objectAddress = buffer.position()+OBJECT_VMEM_SHIFT;
			
			ObjectHeader h = new ObjectHeader();
			try {
				h.loadHeader(buffer);
			} catch (DataLoadException e) {
				System.out.println("Exception @ file pos 0x"+Integer.toHexString( buffer.position() ) );
				e.printStackTrace();
				//break;
				System.exit(EXIT_CODE_ERROR);
			}
			
			if( h.isAllocated() )	allocatedSize += h.getExactSize();
			else					freeSize += h.getExactSize();
			
			if( (h.getRefCount() == 0) && h.isAllocated() )
			{
				System.out.print(String.format("%08x: ", objectAddress));
				h.dump();
			}
			// Here we have object (or free space) information loaded in h, 
			// it's address in Phantom is objectAddress
			
			// store it to hash map
			objects.put(objectAddress, h);
		}
		
		buffer = null; // let GC collect it
		aFile.close();
		// Actually we come here on "phantom.data.DataLoadException: object header marker is wrong"
		// for we can't really tell exact size of Phantom persistent memory. Anyway, lets do GC
		
		//System.out.println("Found "+objects.size()+" objects, allocated .");
		System.out.println( String.format("Found %d objects, allocated %dK, free %dK.", 
				objects.size(), allocatedSize/1024, freeSize/1024));
		System.out.println("Processing.");
		
		// TODO find biggest free slot
		
		// process hash map and do GC
		collectGarbage();
		System.exit( EXIT_CODE_OK );
	}

	private static void collectGarbage() {
		// TODO Auto-generated method stub
		
	}

}
