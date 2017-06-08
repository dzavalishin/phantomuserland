package phantom.data;

import java.nio.ByteBuffer;

public class AllocHeader extends ObjectFlags {

	public static final int PHANTOM_OBJECT_HEADER_SIZE = 16;

	private int refCount;

	private byte allocFlags;
	private byte gcFlags;
	private int exactSize;

	public int getRefCount() { return refCount;	}
	public byte getAllocFlags() {		return allocFlags;	}

	public void setAllocFlag(int flag){
		allocFlags += flag;
	}

	public void removeAllocFlag(int flag){
		allocFlags -= flag;
	}

	//object from that moment is considered to be free
	public void markObjectFree(){
		allocFlags = 0;
	}
	public byte getGcFlags() {		return gcFlags;	}
	public int getExactSize() {		return exactSize;	}
	
	public boolean isAllocated() { return (PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED & allocFlags) != 0; }
	
	protected void loadHeader(ByteBuffer bb) throws DataLoadException
	{
		int objectMarker = bb.getInt();
		if(objectMarker != PVM_OBJECT_START_MARKER) // PVM_OBJECT_START_MARKER = 0x7FAA7F55
		{
			//System.out.println("ObjectHeader.loadMe() marker is wrong @"+Long.toHexString(phantomObjectAddress));
			throw new DataLoadException("object header marker is wrong, = "+Integer.toHexString(objectMarker));
		}

		refCount = bb.getInt();


		allocFlags = bb.get();
		gcFlags = bb.get();

		bb.get();
		bb.get();

		exactSize = bb.getInt();
/*		
		if(exactSize != bb.capacity())
		{	
			//System.out.println("ObjectHeader.loadMe() exact size field is wrong @"+Long.toHexString(phantomObjectAddress));
			throw new DataLoadException("object header marker is wrong");
		}
*/		
	}

}
