/**
 * 
 */
package phantom.gc;

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.stream.Collectors;

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
	//private static final int EXIT_CODE_OK = 0;		// Checked memory and found it to be correct
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

		System.out.println(System.getenv("PHANTOM_HOME"));
		String fn = System.getenv("PHANTOM_HOME") + "run/last_snap.data";

		if( args.length == 1 )
			fn = args[0];
		
		
		System.out.println("Loading " + fn);
		
		try {
			process(fn);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			System.exit(EXIT_CODE_FAILED);
		}

		collectGarbage();


		// writing new .data file
		String output = System.getenv("PHANTOM_HOME") + "tools/big_gc/out.data";
		try {
			saveObjectsToDataFile(translateGarbageMapToList(objects, false), output);
			saveObjectMapToCSVFile(objects, "objects.csv");
		} catch (IOException e) {
			e.printStackTrace();
			System.exit(EXIT_CODE_FAILED);
		}
		System.out.println("Done!");
	}


	private static void process(String fn) throws IOException {
		RandomAccessFile aFile;
		aFile = new RandomAccessFile(fn, "r");
		FileChannel inChannel = aFile.getChannel();

		System.out.println("mem size = "+inChannel.size()/1024+"K");
		//System.out.println("int = "+Integer.toHexString(aFile.readInt()) );

		//MappedByteBuffer buffer;
		ByteBuffer buffer; 
		
		buffer = inChannel.map(FileChannel.MapMode.READ_ONLY, 0, inChannel.size());

		buffer.order(ByteOrder.LITTLE_ENDIAN);
		System.out.println(String.format("Size of buffer while reading = %d", buffer.capacity()));

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

		int size = 0;
		for( ObjectHeader header : objects.values()){
			size += header.getExactSize();
		}
		System.out.println(String.format("Exact size just after reading: %d", size));

		buffer = null; // let GC collect it
		aFile.close();
		// Actually we come here on "phantom.data.DataLoadException: object header marker is wrong"
		// for we can't really tell exact size of Phantom persistent memory. Anyway, lets do GC
		
		//System.out.println("Found "+objects.size()+" objects, allocated .");
		System.out.println( String.format("Found %d objects, allocated %dK, free %dK.", 
				objects.size(), allocatedSize/1024, freeSize/1024));
		System.out.println("Processing.");

		// TODO find biggest free slot
	}

	private static void collectGarbage() {
		// TODO Auto-generated method stub


		// address of the very first object
		// now it is 2147483648
		long rootObjectAddress = OBJECT_VMEM_SHIFT;

		Queue<Long> objectsToInspect = new LinkedList<>();
		Map<Long, ObjectHeader> visitedObjects = new HashMap<>();

		objectsToInspect.add(rootObjectAddress);
		ObjectHeader currentObject;
		Long currentObjectAddress;


		while (objectsToInspect.size() != 0){
			currentObjectAddress = objectsToInspect.remove();
			// reference isn't initialized yet
			if(currentObjectAddress == 0) continue;


			currentObject = objects.get(currentObjectAddress);
			
			if(currentObject == null)
			{
				System.err.println("object mising @"+currentObjectAddress);
				continue;
			}
			
			visitedObjects.put(currentObjectAddress, currentObject);

			// inspect object
			// for external objects check whether there are
			// references to objects that hasn't been visited yet
			// if so -> add them to the queue
			if(!currentObject.isInternal()){
				ByteBuffer bb =  currentObject.getDataArea();

				// some objects has value "null" or "0"
				// for almost all fields. In this case
				// we need to check whether dataArea field exist or not
				// if not -> skip this object
				if(bb == null)
					continue;

				List<Long> refs = getListOfObjectPointers(bb);
				for(Long ref : refs){
					if(visitedObjects.get(ref) == null)
						objectsToInspect.add(ref);
				}
			}else{
				// for internal objects
				// read 4 bytes with step 1 byte
				// and check whether there is an object
				// by this address.
				// If so - process it, skip otherwise
				ByteBuffer da = currentObject.getDataArea();
				da.rewind();
				long ref;
				byte[] bb = new byte[4];
				for(int i = 0; i < da.capacity() - 4; i++){
					da.position(i);
					da.get(bb, 0, 4);
					ref = byteArrayToInt(bb);
					if(objects.get(ref) != null && visitedObjects.get(ref) == null)
						objectsToInspect.add(ref);
				}
			}
		}

		// print some statistics and save all garbage objects to a file
		// file will be saved to /analysis/ folder
		printGarbageStatistics(visitedObjects, objects);
		try {
			saveObjectMapToCSVFile(getGarbage(visitedObjects, objects), "garbageObjects.csv");
		} catch (IOException e) {
			e.printStackTrace();
		}

		for(Map.Entry<Long,ObjectHeader> entry: objects.entrySet()){
			//if object isn't garbage -> don't do anything
			// otherwise mark it as free
			if(visitedObjects.get(entry.getKey()) == null){
				entry.getValue().markObjectFree();
			}
		}
	}


	public static Map<Long, ObjectHeader> collectGarbage(Map<Long, ObjectHeader> objects){
		Main.objects = objects;
		Main.collectGarbage();
		return Main.objects;
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
	public static void printStatisticsPerSystem(Map<Long, ObjectHeader> objectsToInspect){
		System.out.println("Printing statistics");
		int internal = 0;
		int external = 0;
		int freeInternal = 0;
		int freeExternal = 0;

		for(ObjectHeader value : objectsToInspect.values()){
			if(value.isAllocated()){
				if(value.isInternal()){
					internal++;
				}else{
					external++;
				}
			}else{
				if(value.isInternal()){
					freeInternal++;
				}else{
					freeExternal++;
				}
			}
		}
		System.out.println(String.format("  Internal: %d, external: %d, freeInternal: %d, freeExternal: %d,  all: %d",
				internal, external, freeInternal,freeExternal, internal + external + freeExternal + freeInternal));
	}


	/**
	 * Prints how many non-garbage was found and distribution depending on
	 * the amount of refCounters
	 * @param withoutGarbage Map<Long, ObjectHeader> objects after gc
	 * @param withGarbage Map<Long, ObjectHeader> objects before gc
	 */
	public static void printGarbageStatistics(Map<Long, ObjectHeader> withoutGarbage,
											Map<Long, ObjectHeader> withGarbage){

		System.out.println("Printing garbage statistics:");
		Map<Long, ObjectHeader> garbage = getGarbage(withoutGarbage, withGarbage);

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
		System.out.println(String.format("    0: %d objects", refCounterDistribution.get(0)));
		System.out.println(String.format("    1: %d objects", refCounterDistribution.get(1)));
		System.out.println(String.format("  2-5: %d objects", refCounterDistribution.get(2)));
		System.out.println(String.format("   >5: %d objects", refCounterDistribution.get(3)));
	}

	/**
	 * convert array of bytes to unsigned int
	 * 	array must be in big endian order
	 */
	private static long byteArrayToInt(byte[] bytes) {
		return (long)((bytes[3] << 24) | (bytes[2] & 0xFF) << 16 | (bytes[1] & 0xFF) << 8 | (bytes[0] & 0xFF)) & 0xffffffffL;
	}


	/**
	 * save objects to file in csv format
	 * needed for analyzing usage of objects using jupyter or other tools
	 * @param objectsMap Map<Long, ObjectHeader>
	 */
	public static void saveObjectMapToCSVFile(Map<Long, ObjectHeader> objectsMap, String filename)
			throws IOException {

		//csv header, has to be written to file first
		String csvHeader = "objAddress,isAllocated,isInternal,refCount,allocFlags,gcFlags" +
				",exactSize,oClass,oSatellites,objectFlags,daSize,dataArea";


		// each row corresponds to one and only one object
		List<String> csvRows = new ArrayList<>(objectsMap.size() + 1);
		csvRows.add(csvHeader);

		int isInternal, isAllocated;


		// iterating over all objects in the map
		// and creating rows for all of them

		// isInternal has value 1 if object is internal and 0 otherwise
		// isAllocated has value 1 if object is allocated and 0 otherwise
		// oClass and oSatellites corresponds to the result of ObjectRef.toString() method now
		// dataArea corresponds to a string written in a like ref1;ref2;ref3...
		for(Map.Entry<Long,ObjectHeader> entry : objectsMap.entrySet()){
			List<String> row = new ArrayList<>();

			row.add(Long.toString(entry.getKey()));

			if(entry.getValue().isAllocated())
				isAllocated = 1;
			else
				isAllocated = 0;
			row.add(Integer.toString(isAllocated));

			if(entry.getValue().isInternal())
				isInternal = 1;
			else
				isInternal = 0;
			row.add(Integer.toString(isInternal));

			row.add(Integer.toString(entry.getValue().getRefCount()));
			row.add(Integer.toString(entry.getValue().getAllocFlags()));
			row.add(Integer.toString(entry.getValue().getGcFlags()));
			row.add(Integer.toString(entry.getValue().getExactSize()));
			if(entry.getValue().getClassRef() != null)
				row.add(entry.getValue().getClassRef().toString());
			else
				row.add("0");

			if(entry.getValue().getObjectSatellites() != null)
				row.add(entry.getValue().getClassRef().toString());
			else
				row.add("0");

			row.add(Integer.toString(entry.getValue().getObjectFlags()));
			row.add(Integer.toString(entry.getValue().getDaSize()));

			if(isInternal == 0 && isAllocated == 1 && entry.getValue().getDaSize() != 0){
				List<Long> refs = getListOfObjectPointers(entry.getValue().getDataArea());
				List<String> stringRefs = refs.stream()
						.map(Object::toString)
						.collect(Collectors.toList());
				row.add(String.join(";", stringRefs));
			}else{
				row.add("null");
			}

			csvRows.add(String.join(",", row));
		}



		Path file = Paths.get(System.getenv("PHANTOM_HOME") + "tools/big_gc/analysis/" + filename);
		String result = String.join("\n", csvRows);
		BufferedWriter bf = Files.newBufferedWriter(file, Charset.forName("UTF-8"));
		bf.write(result);
		bf.close();
	}


	/**
	 * Get Map of garbage objects
	 * @param withoutGarbage Map<Long, ObjectHeader>
	 * @param withGarbage Map<Long, ObjectHeader>
	 * @return (objects that exist in withoutGarbage and don't exist in withGarbage)
	 */
	private static Map<Long, ObjectHeader> getGarbage(Map<Long, ObjectHeader> withoutGarbage,
													  Map<Long, ObjectHeader> withGarbage){

		Map<Long, ObjectHeader> garbage = new HashMap<>();
		for(Map.Entry<Long,ObjectHeader> entry: withGarbage.entrySet()){
			if(entry.getValue().isAllocated() && (withoutGarbage.get(entry.getKey()) == null)){
				garbage.put(entry.getKey(), entry.getValue());
			}
		}
		return garbage;
	}

	/**
	 * Translate Map<Long, ObjectHeader> to List<ObjectHeader>
	 *     and collapses free objects if needToCollapse is true
	 * @return List of ObjectHeader objects
	 */
	private static List<ObjectHeader> translateGarbageMapToList(Map<Long, ObjectHeader> objectHeaderMap, boolean needToCollapse){
		List<Map.Entry<Long, ObjectHeader>> entries = new ArrayList<>(objectHeaderMap.entrySet());

		entries.sort((a, b) -> Long.compare(b.getKey(), a.getKey()));
		List<ObjectHeader> result = new ArrayList<>();



		if(!needToCollapse){
			for(Map.Entry<Long, ObjectHeader> entry : entries)
				result.add(entry.getValue());
			return result;
		}

		ObjectHeader prevFreeObj = null;
		// collapse free objects in one
		for(Map.Entry<Long, ObjectHeader> entry : entries){
			if(entry.getValue().isAllocated()){
				result.add(entry.getValue());
				if(prevFreeObj != null){
					result.add(prevFreeObj);
					prevFreeObj = null;
				}
			}else{
				if(prevFreeObj != null){
					prevFreeObj.setExactSize(prevFreeObj.getExactSize() + entry.getValue().getExactSize());
				}else{
					prevFreeObj = entry.getValue();
				}
			}
		}
		if(prevFreeObj != null)
			result.add(prevFreeObj);

		return result;
	}


	/**
	 * Save list of objects to data file.
	 * phantom.img ->(using pfsexstruct tool) last_snap.data -> Map<Long, ObjectHeader>
	 * List<ObjectHeader> -> last_snap.data ->(using some tool) phantom.img
	 */
	private static void saveObjectsToDataFile(List<ObjectHeader> headers, String fileName)
			throws IOException {
		//determine how big byteBuffer has to be
		int size = 0;
		for(ObjectHeader header: headers){
			size += header.getExactSize();
		}
		System.out.println(String.format("Size of byteBuffer to write is %d", size));

		RandomAccessFile outputFile = new RandomAccessFile(fileName, "rw");
		ByteBuffer buffer = ByteBuffer.allocate(size);

		buffer.order(ByteOrder.LITTLE_ENDIAN);
		for(ObjectHeader header : headers){
			header.writeToByteBuffer(buffer);
		}
		outputFile.write(buffer.array());
		outputFile.close();
	}



}
