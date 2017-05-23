package phantom.data;

import java.nio.ByteBuffer;

public class ObjectHeader extends AllocHeader {

	private ObjectRef oClass;
	private ObjectRef oSatellites;
	private int objectFlags;
	private int daSize;
	private ByteBuffer dataArea;
	

	//public ObjectRef getoClass() {		return oClass;	}
	//public ObjectRef getoSatellites() {		return oSatellites;	}

	public ObjectRef getClassRef() {		return oClass;	}
	public ObjectRef getObjectSatellites() {	return oSatellites; }

	public int getObjectFlags() {		return objectFlags;	}
	public int getDaSize() {		return daSize;	}

	public String getFlagsList() { return getFlagsList(objectFlags); }

	@Override
	protected void loadHeader(ByteBuffer bb) throws DataLoadException {
		super.loadHeader(bb);

		oClass = new ObjectRef(bb);
		oSatellites = new ObjectRef(bb);
		objectFlags = bb.getInt();
		daSize = bb.getInt();
		
		byte[] daArray = new byte[daSize];
		bb.get(daArray, 0, daSize);
		
		dataArea = ByteBuffer.wrap(daArray).asReadOnlyBuffer();
	}
	
	public ByteBuffer getDataArea() {
		return dataArea;
	}
	
	
	public boolean isInternal() 
	{
		// interface da is the same as for usual object, treat it as not internal	
		return 
				((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL) != 0) && 
				((objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE) == 0);
	}
	
	
}
