package phantom.data;

import java.nio.ByteBuffer;

public class AllocHeader extends ObjectFlags {

	public static final int PHANTOM_OBJECT_HEADER_SIZE = 16;

	private int refCount;

	private byte allocFlags;
	private byte gcFlags;
	private int exactSize;

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

	// based on loadHeader method
	protected void writeToByteBuffer(ByteBuffer byteBuffer){
		byteBuffer.putInt(PVM_OBJECT_START_MARKER);
		byteBuffer.putInt(this.refCount);
		byteBuffer.put(allocFlags);
		byteBuffer.put(gcFlags);

		byteBuffer.put((byte)0x00);
		byteBuffer.put((byte)0x00);

		byteBuffer.putInt(this.exactSize);
	}

	// setters and getters for all fields
	// needed to create such objects while
	// garbage is being generated


	public int getRefCount() {
		return refCount;
	}

	public void setRefCount(int refCount) {
		this.refCount = refCount;
	}

	public byte getAllocFlags() {
		return allocFlags;
	}

	public void setAllocFlags(byte allocFlags) {
		this.allocFlags = allocFlags;
	}

	public byte getGcFlags() {
		return gcFlags;
	}

	public void setGcFlags(byte gcFlags) {
		this.gcFlags = gcFlags;
	}

	public int getExactSize() {
		return exactSize;
	}

	// has to be used very carefully
	// could make snapshot invalid
	public void setExactSize(int exactSize) {
		this.exactSize = exactSize;
	}

}
