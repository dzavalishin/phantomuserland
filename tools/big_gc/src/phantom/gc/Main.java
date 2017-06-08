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
import java.util.*;

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

	private static Map<Long,ObjectHeader> objects = new HashMap<Long,ObjectHeader>();
	private static long allocatedSize = 0;
	private static long freeSize = 0;

	/**
	 * @param args String[]
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


		System.out.println("Before garbage collecting:");
		printStatisticsPerSystem(objects);

		System.out.println("Let's collect garbage");

		// process hash map and do GC
		collectGarbage();

		System.out.println("After garbage collecting:");
		printStatisticsPerSystem(objects);


		System.exit( EXIT_CODE_OK );
	}

	private static void collectGarbage() {
		// TODO Auto-generated method stub


		// address of the very first object
		// now it is 2147483648
		long rootObjectAddress = OBJECT_VMEM_SHIFT;

		Queue<Long> objectsToInspect = new LinkedList<>();
		HashMap<Long, ObjectHeader> visitedObjects = new HashMap<>();
		//HashSet<Long> visitedObjects = new HashSet<>();

		objectsToInspect.add(rootObjectAddress);
		ObjectHeader currentObject;
		Long currentObjectAddress;


		while (objectsToInspect.size() != 0){
			currentObjectAddress = objectsToInspect.remove();
			// reference isn't initialized yet
			if(currentObjectAddress == 0) continue;


			currentObject = objects.get(currentObjectAddress);
			visitedObjects.put(currentObjectAddress, currentObject);


			// inspect object
			// for external objects check whether there are
			// references to objects that hasn't been visited yet
			// if so -> add them to the queue
			// since internal objects has no references
			// to another objects -> just skip them
			if(!currentObject.isInternal()){
				List<Long> refs = getListOfObjectPointers(currentObject.getDataArea());
				for(Long ref : refs){
					if(visitedObjects.get(ref) == null)
						objectsToInspect.add(ref);
				}
			}
		}

		printGarbageStatistics(visitedObjects, objects);

		for(Map.Entry<Long,ObjectHeader> entry: objects.entrySet()){
			//if object isn't garbage -> don't do anything
			// otherwise mark it as free
			if(visitedObjects.get(entry.getKey()) == null){
				entry.getValue().markObjectFree();
			}
		}
	}


	/**
	 * Parses da[] field and returns a list of references to objects
	 * @param buffer ByteBuffer da[] value
	 * @return list of references(list of Long)
	 */
	private static List<Long> getListOfObjectPointers(ByteBuffer buffer){
		ArrayList<Long> listOfPointers = new ArrayList<>();
		byte[] bb = new byte[4];
		long v;

		buffer.rewind();
		while(buffer.position() < buffer.capacity()){
			buffer.get(bb, 0, 4);
			v = byteArrayToInt(bb);
			listOfPointers.add(v);
		}

		return listOfPointers;
	}


	/**
	 * Prints how many internal, external and free objects there are in the system and sum of all of them
	 * @param objectsToInspect Map<Long, ObjectHeader> map of objects existing in the system
	 */
	private static void printStatisticsPerSystem(Map<Long, ObjectHeader> objectsToInspect){
		int internal = 0;
		int external = 0;
		int free = 0;

		for(ObjectHeader value : objectsToInspect.values()){
			if(value.isAllocated()){
				if(value.isInternal()){
					internal++;
				}else{
					external++;
				}
			}else{
				free++;
			}
		}
		System.out.println(String.format("  Internal: %d, external: %d, free: %d, all: %d", internal, external, free, internal + external + free));
	}


	/**
	 * Prints how many non-garbage was found and distribution depending on
	 * the amount of refCounters(Now for refCount == 0, refCount == 1, 2<=refCount<=5 , refCount>=5)
	 * @param withoutGarbage Map<Long, ObjectHeader> objects after gc
	 * @param withGarbage Map<Long, ObjectHeader> objects before gc
	 */
	private static void printGarbageStatistics(Map<Long, ObjectHeader> withoutGarbage,
											Map<Long, ObjectHeader> withGarbage){

		System.out.println("Printing garbage statistics:");
		HashMap<Long, ObjectHeader> garbage = new HashMap<>();

		for(Map.Entry<Long,ObjectHeader> entry: withGarbage.entrySet()){
			if(entry.getValue().isAllocated() && (withoutGarbage.get(entry.getKey()) == null)){
				garbage.put(entry.getKey(), entry.getValue());
			}
		}
		System.out.println(String.format("  Found %d garbage objects", garbage.size()));



		//[0] - 0 refCount
		//[1] - 1 refCount
		//[2] - 2-5 refCount
		//[3] - more
		ArrayList<Integer> refCounterDistribution = new ArrayList<>(Collections.nCopies(4, 0));
		for(ObjectHeader objectHeader: garbage.values()){
			if(objectHeader.getRefCount() == 0){
				refCounterDistribution.set(0, refCounterDistribution.get(0) + 1);
			}else if(objectHeader.getRefCount() == 1){
				refCounterDistribution.set(1, refCounterDistribution.get(1) + 1);
			}else if(objectHeader.getRefCount() >=2 && objectHeader.getRefCount() <= 5){
				refCounterDistribution.set(2, refCounterDistribution.get(2) + 1);
			}else{
				refCounterDistribution.set(3, refCounterDistribution.get(3) + 1);
			}
		}

		System.out.println("Distribution:");
		System.out.println(String.format("  0: %d", refCounterDistribution.get(0)));
		System.out.println(String.format("  1: %d", refCounterDistribution.get(1)));
		System.out.println(String.format("  2-5: %d", refCounterDistribution.get(2)));
		System.out.println(String.format("  >5: %d", refCounterDistribution.get(3)));


	}

	/**
	 * convert array of bytes in unsigned int
	 * 	array must be in big endian order
	 */
	private static long byteArrayToInt(byte[] bytes) {
		return (long)((bytes[3] << 24) | (bytes[2] & 0xFF) << 16 | (bytes[1] & 0xFF) << 8 | (bytes[0] & 0xFF)) & 0xffffffffL;
	}


	private static void analyse

}
