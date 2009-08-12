package ru.dz.phantom.snapdump;

import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;
import java.util.Vector;

/**
 * struct pvm_object_storage
 * {
 *     struct object_PVM_ALLOC_Header	_ah;
 *     
 *     struct pvm_object  		        _class;
 *     unsigned char			_flags; // TODO int! Need more flags
 *     unsigned int			_da_size; // in bytes!
 *     
 *     unsigned char                       da[];
 *     };
 *     
 * @author dz
 *
 */
public class ObjectHeader {

	// TODO - hardcoded size
	private static final int REF_BYTES = 8;
	private ObjectRef classRef;
	private byte objectFlags;
	private int daSize;

	Vector<ObjectRef> refs = new Vector<ObjectRef>();
	
	/**
	 * 
	 * @param bb buffer to load from
	 * @param addr start address of object
	 */
	public ObjectHeader(MappedByteBuffer bb, int addr) {
		bb.position(addr);
		loadMe(bb); 
	}

	/**
	 * 
	 * @param bb buffer to load from
	 * @param addr start address of object
	 */
	public ObjectHeader(MappedByteBuffer bb) {
		loadMe(bb); 
	}

	private void loadMe(MappedByteBuffer bb) {
		
		classRef = new ObjectRef(bb);
		objectFlags = bb.get();
		bb.get(); // align
		bb.get();
		bb.get();
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
				//System.out.println("ObjectHeader.loadMe() skip unknown internal");
				loadBinary(bb);
			}
		}
		else
		{
			int nrefs = daSize / REF_BYTES;
			while(nrefs-- > 0)
				refs.add( new ObjectRef(bb) );
		}
	}


	public ObjectRef getClassRef() {		return classRef;	}

	public byte getObjectFlags() {		return objectFlags;	}

	public int getDaSize() {		return daSize;	}

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
	
	private void loadBinary(MappedByteBuffer bb) {
		byte [] buf = new byte[daSize];
		
		bb.get(buf); // load all the da
		
		ByteBuffer wrap = ByteBuffer.wrap(buf);

		for( int pos = 0; pos < daSize-4; pos++)
		{
			int addr = ObjectRef.convertAddress( wrap.getInt(pos) );
			
			try {
				// try to access it.
				new AllocHeader(bb, addr);
				//Ok?
				refs.add( new ObjectRef(addr, 0) );
				System.out.println("ObjectHeader.loadBinary() scanned ref "+addr);
			}
			catch(InvalidPhantomMemoryException e)
			{
				// ignore, it was not a ref
				continue;
			}
			
		}
		
	}

	
}


