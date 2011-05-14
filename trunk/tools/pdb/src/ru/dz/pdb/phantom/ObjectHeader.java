package ru.dz.pdb.phantom;

import java.nio.ByteBuffer;
import java.util.Vector;

public class ObjectHeader {

	public final int PVM_OBJECT_START_MARKER = 0x7FAA7F55;
	private static final int REF_BYTES = 8; // TODO - hardcoded size
	
	private ByteBuffer bb;

	private int objectRefCount;
	private byte allocFlags;
	private byte gcFlags;
	private int exactSize;

	private ObjectRef oClass;
	private ObjectRef oSatellites;
	private int objectFlags;
	private int daSize;


	private Vector<ObjectRef> refs = new Vector<ObjectRef>();

	
	
	
	public ObjectHeader(byte [] data) {
		bb = ByteBuffer.wrap(data);
		loadMe();
	}

	private void loadMe() 
	{
		//classRef = new ObjectRef(bb);
		int objectMarker = bb.getInt();
		if(objectMarker != PVM_OBJECT_START_MARKER)
			System.out.println("ObjectHeader.loadMe() marker is wrong");
		
		objectRefCount = bb.getInt();

		
		allocFlags = bb.get();
		gcFlags = bb.get();

		bb.get();
		bb.get();
		
		exactSize = bb.getInt();
		if(exactSize != bb.capacity())
			System.out.println("ObjectHeader.loadMe() exact size field is wrong");

		// Now real object hdr
		
		oClass = new ObjectRef(bb);
		oSatellites = new ObjectRef(bb);
		objectFlags = bb.getInt();
		daSize = bb.getInt();

		
		if(isInternal())
		{
			if((objectFlags & 0x20) != 0)
			{
				// String
			}
			else if((objectFlags & 0x10) != 0)
			{
				// Int
			}
			/*else if((objectFlags & 0x04) != 0)
			{
				// class
				byte classFlags = bb.get();
				bb.get(); // 
				bb.get();
				bb.get();
				int classDaSize = bb.getInt();
				ObjectRef classIface = new ObjectRef(bb);
				int classSysId = bb.getInt();
				ObjectRef className = new ObjectRef(bb);
				ObjectRef classParent = new ObjectRef(bb);
				
				refs.add(classIface);
				refs.add(className);
				refs.add(classParent);
			}*/
			else if((objectFlags & 0x01) != 0)
			{
				// Code
			}
			else
			{
				System.out.println("ObjectHeader.loadMe() skip unknown internal");
				//loadBinary(bb);
			}
		}
		else
		{
			int nrefs = daSize / REF_BYTES;
			while(nrefs-- > 0)
				refs.add( new ObjectRef(bb) );
		}
	}

	
	public boolean isInternal() {
		// 0x02 is interface and its da is the same as for usual object
		return ((objectFlags & 0x80) != 0) && ((objectFlags & 0x02) == 0);
	}

	/**
	 *
	 * @return number of object references (fields) in this object.
	 * @throws InvalidObjectOperationException If object is internal
	 */
	public int getDaRefsCount() throws InvalidObjectOperationException {
		if(isInternal())
			throw new InvalidObjectOperationException();
		return daSize / REF_BYTES;
	}
	
	/**
	 *
	 * @return Object references (fields) in this object. Any references!
	 */
	public Vector<ObjectRef> getDaRefs() {
		//if(isInternal())			throw new InvalidObjectOperationException();
		return refs;
	}
	
	
	public ObjectRef getClassRef() {		return oClass;	}

	public int getObjectFlags() {		return objectFlags;	}

	public int getDaSize() {		return daSize;	}
	
}
