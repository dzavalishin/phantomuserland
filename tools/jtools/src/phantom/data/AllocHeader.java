package phantom.data;

import java.nio.ByteBuffer;

public class AllocHeader extends ObjectFlags {

	private int refCount;

	private byte allocFlags;
	private byte gcFlags;
	private int exactSize;

	public int getRefCount() { return refCount;	}
	public byte getAllocFlags() {		return allocFlags;	}
	public byte getGcFlags() {		return gcFlags;	}
	public int getExactSize() {		return exactSize;	}
	
	protected void loadHeader(ByteBuffer bb) throws DataLoadException
	{
		int objectMarker = bb.getInt();
		if(objectMarker != PVM_OBJECT_START_MARKER)
		{
			//System.out.println("ObjectHeader.loadMe() marker is wrong @"+Long.toHexString(phantomObjectAddress));
			throw new DataLoadException("object header marker is wrong");
		}

		refCount = bb.getInt();


		allocFlags = bb.get();
		gcFlags = bb.get();

		bb.get();
		bb.get();

		exactSize = bb.getInt();
		if(exactSize != bb.capacity())
		{	
			//System.out.println("ObjectHeader.loadMe() exact size field is wrong @"+Long.toHexString(phantomObjectAddress));
			throw new DataLoadException("object header marker is wrong");
		}
	}

}
