package ru.dz.phantom.snapdump;

import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;


/**
 * Phantom memory allocator header representation. Keep in sync with pvm_object.h.
 * @author dz
 *
 *  struct object_PVM_ALLOC_Header
 *  {
 *      unsigned int		object_start_marker;
 *      unsigned char		alloc_flags;
 *      unsigned char		gc_flags;
 *      unsigned int		exact_size; // full object size including this header
 *      //unsigned char     usage_count; //
 *  };
 *  
 */
public class AllocHeader {
	//public static int PVM_OBJECT_START_MARKER = 0x557FAA7F;
	public static int PVM_OBJECT_START_MARKER = 0x7FAA7F55;
	
	public static int PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED = 0x01;

	
	private int refCount;
	private byte allocFlags;
	private byte gcFlags;
	private int size;


	/**
	 * 
	 * @param bb buffer to load from
	 * @param addr start address of object
	 * @throws InvalidPhantomMemoryException if memory header magic is invalid
	 */
	public AllocHeader(MappedByteBuffer bb, int addr) throws InvalidPhantomMemoryException {
		bb.position(addr);
		
		int marker = bb.getInt();
		
		if(marker != PVM_OBJECT_START_MARKER)
			throw new InvalidPhantomMemoryException();
		
		refCount = bb.getInt();
		
		allocFlags = bb.get();
		gcFlags = bb.get();
		bb.get(); // align 4
		bb.get();
		size = bb.getInt();	
	}

	/**
	 * 
	 * @param bb buffer to load from
	 * @param addr start address of object
	 * @throws InvalidPhantomMemoryException if memory header magic is invalid
	 */
	public AllocHeader(MappedByteBuffer bb) throws InvalidPhantomMemoryException {
		int pos = bb.position();
		
		int marker = bb.getInt();
		
		if(marker != PVM_OBJECT_START_MARKER)
			throw new InvalidPhantomMemoryException("No object header at "+pos);

		refCount = bb.getInt();
		
		allocFlags = bb.get();
		gcFlags = bb.get();
		bb.get(); // align 4
		bb.get();
		size = bb.getInt();
		
	}
	
	public int getRefCount() {		return refCount;	}
	public byte getAllocFlags() {		return allocFlags;	}
	public byte getGcFlags() {		return gcFlags;	}
	
	/**
	 * @return exact object size in bytes.
	 */
	public int getSize() {		return size;	}

	public boolean isFree() {
		return (allocFlags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED) == 0;	
	}

	
}
