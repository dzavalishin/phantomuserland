package ru.dz.pdb.phantom;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Vector;

import phantom.data.DataLoadException;
import phantom.data.ObjectRef;
import ru.dz.pdb.Main;

public class ObjectHeader extends phantom.data.ObjectHeader {




	private final long phantomObjectAddress;

	private ByteBuffer in_bb;




	private Vector<ObjectRef> refs = new Vector<ObjectRef>();
	private final byte[] data;
	private String asString;

	public ObjectHeader(byte [] data, long phantomObjectAddress) {
		this.data = data;
		this.phantomObjectAddress = phantomObjectAddress;
		in_bb = ByteBuffer.wrap(data);
		in_bb.order(ByteOrder.LITTLE_ENDIAN);
		loadMe();
	}

	private void loadMe() 
	{
		try {
			loadHeader(in_bb);
		} catch (DataLoadException e1) {
			System.out.println("ObjectHeader.loadMe() "+e1.toString()+" @"+Long.toHexString(phantomObjectAddress));
			//e1.printStackTrace();
		}
		//classRef = new ObjectRef(bb);
		




		ByteBuffer bb = getDataArea();
		if(isInternal())
		{
			if((getObjectFlags() & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING) != 0)
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
			else if((getObjectFlags() & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT) != 0)
			{
				// Int
			}
			else if((getObjectFlags() & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE) != 0)
			{
				// Code
			}
			else
			{
				//System.out.println("ObjectHeader.loadMe() skip unknown internal");
				//loadBinary(bb);
			}
		}
		else
		{
			int nrefs = getDaSize() / REF_BYTES;
			while(nrefs-- > 0)
				refs.add( new ObjectRef(bb) );
		}
	}

	// TODO is it right?
	private int getDaOffset() {
		return data.length-getDaSize();
	}

	public byte [] getDaData()
	{
		byte [] ret = new byte[getDaSize()];
		System.arraycopy(data, getDaOffset(), ret, 0, getDaSize());
		return ret;
	}




	/**
	 *
	 * @return number of object references (fields) in this object.
	 * @throws InvalidObjectOperationException If object is internal
	 */
	public int getDaRefsCount() throws InvalidObjectOperationException {
		if(isInternal())
			throw new InvalidObjectOperationException();
		return getDaSize() / REF_BYTES;
	}

	/**
	 *
	 * @return Object references (fields) in this object. Any references!
	 */
	public Vector<ObjectRef> getDaRefs() {
		//if(isInternal())			throw new InvalidObjectOperationException();
		return refs;
	}



	//public int getObjectFlags() {		return objectFlags;	}

	//public int getDaSize() {		return daSize;	}

	// If object is a string, return data
	public String getAsString() {		return asString;	}



	public IKnownType getAvatar() throws InvalidObjectOperationException
	{
		int objectFlags = getObjectFlags();
		if(isInternal())
		{
			if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING) != 0)
			{
				return new StringObject(this);			
				//throw new InvalidObjectOperationException("No avatar for string internal");			
			}
			else if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT) != 0)
			{
				// Int
				throw new InvalidObjectOperationException("No avatar for int internal");			
			}
			else if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS ) != 0)
			{
				return new ClassObject(this); 
			}
			else if((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE) != 0)
			{
				// Code
				return new CodeObject(this); 
				//throw new InvalidObjectOperationException("No avatar for unknown internal");			
			}
			else
			{
				//System.out.println("unknown internal");

			
				ClassObject classObject = Main.getPhantomClass(getClassRef());
				
				if(classObject == null)			
					throw new InvalidObjectOperationException("No avatar for unknown internal, flags "+Integer.toHexString(getObjectFlags()));			
				
				int sysTableId = classObject.getSysTableId();
				
				if(sysTableId == 0)
					throw new InvalidObjectOperationException("Void object?");
				
				switch(sysTableId)
				{
				case 6:
					//System.out.println("array");
					return new ArrayObject(this);
					
				case 8: 
					return new ThreadObject(this);
					
				case 9: 
					return new CallFrameObject(this);

				case 10: 
					return new IStackObject(this);
				
				case 11: 
					return new OStackObject(this);
					
				case 12: 
					return new EStackObject(this);
					
				default:
					throw new InvalidObjectOperationException("Unknown sys id "+sysTableId);
				}
			}
		}
		else
		{
			throw new InvalidObjectOperationException("No avatar");
		}

	}

}
