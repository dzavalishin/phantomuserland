package ru.dz.pdb.phantom;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Vector;

public class ObjectHeader {

	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_FINALIZER 		= 0x1000;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE  	= 0x800;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD  		= 0x400;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME 	= 0x200;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME 	= 0x100;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL 		= 0x80;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE 	= 0x40;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING 		= 0x20;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_INT  			= 0x10;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_DECOMPOSEABLE 	= 0x08;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS 			= 0x04;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE 		= 0x02;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE  			= 0x01;

	public final int PVM_OBJECT_START_MARKER = 0x7FAA7F55;
	private static final int REF_BYTES = 8; // TODO - hardcoded size



	private final long phantomObjectAddress;

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
	private final byte[] data;
	private String asString;

	public ObjectHeader(byte [] data, long phantomObjectAddress) {
		this.data = data;
		this.phantomObjectAddress = phantomObjectAddress;
		bb = ByteBuffer.wrap(data);
		bb.order(ByteOrder.LITTLE_ENDIAN);
		loadMe();
	}

	private void loadMe() 
	{
		//classRef = new ObjectRef(bb);
		int objectMarker = bb.getInt();
		if(objectMarker != PVM_OBJECT_START_MARKER)
			System.out.println("ObjectHeader.loadMe() marker is wrong @"+Long.toHexString(phantomObjectAddress));

		objectRefCount = bb.getInt();


		allocFlags = bb.get();
		gcFlags = bb.get();

		bb.get();
		bb.get();

		exactSize = bb.getInt();
		if(exactSize != bb.capacity())
			System.out.println("ObjectHeader.loadMe() exact size field is wrong @"+Long.toHexString(phantomObjectAddress));

		// Now real object hdr

		oClass = new ObjectRef(bb);
		oSatellites = new ObjectRef(bb);
		objectFlags = bb.getInt();
		daSize = bb.getInt();


		if(isInternal())
		{
			if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING) != 0)
			{
				// String
				//int offset = getDaOffset();
				int stringSize = bb.getInt();
				byte[] dst = new byte[stringSize]; 
				bb.get(dst);
				try {
					asString = new String(dst,"UTF-8");
				} catch (UnsupportedEncodingException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
					asString = null;
				}
			}
			else if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT) != 0)
			{
				// Int
			}
			else if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE) != 0)
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


	private int getDaOffset() {
		return data.length-daSize;
	}

	public byte [] getDaData()
	{
		byte [] ret = new byte[daSize];
		System.arraycopy(data, getDaOffset(), ret, 0, daSize);
		return ret;
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
	public ObjectRef getObjectSatellites() {	return oSatellites; }

	public int getObjectFlags() {		return objectFlags;	}

	public int getDaSize() {		return daSize;	}

	// If object is a string, return data
	public String getAsString() {		return asString;	}


	public String getFlagsList() {
		StringBuilder sb = new StringBuilder();

		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_FINALIZER))		sb.append("Fin "); 		
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE))  	sb.append("NoChild ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD))  		sb.append("Thread ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME)) 	sb.append("SFrame ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME)) 	sb.append("CFrame ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL)) 		sb.append("Internal ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE)) 	sb.append("Resize ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING)) 		sb.append("String ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT))  			sb.append("Int ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_DECOMPOSEABLE)) 	sb.append("Decomp ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS)) 			sb.append("Class ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE)) 		sb.append("IFace ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE))  			sb.append("Code ");

		return sb.toString();
	}

	public IKnownType getAvatar() throws InvalidObjectOperationException
	{
		if(isInternal())
		{
			if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING) != 0)
			{
				throw new InvalidObjectOperationException("No avatar for unknown internal");			
			}
			else if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT) != 0)
			{
				// Int
				throw new InvalidObjectOperationException("No avatar for unknown internal");			
			}
			else if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS ) != 0)
			{
				return new ClassObject(this); 
			}
			else if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE) != 0)
			{
				// Code
				throw new InvalidObjectOperationException("No avatar for unknown internal");			
			}
			else
			{
				System.out.println("unknown internal");
				throw new InvalidObjectOperationException("No avatar for unknown internal");			
			}
		}
		else
		{
			throw new InvalidObjectOperationException("No avatar"); 
		}

	}
}
