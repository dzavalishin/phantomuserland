package phantom.data;

import java.nio.ByteBuffer;




/**
 * struct pvm_object
 * {
 *     struct pvm_object_storage	* data;
 *     struct pvm_object_storage	* interface; // method list is here
 * };
 * 
 * @author dz
 *
 */
public class ObjectRef {
	private long dataAddr;
	//private long interfaceAddr;

	/**
	 * 
	 * @param bb buffer to load from
	 * @param pos start position of object
	 */
	public ObjectRef(ByteBuffer bb, int pos) {
		bb.position(pos);
		
		dataAddr = bb.getInt();
		//interfaceAddr = bb.getInt();		
	}

	/**
	 * 
	 * @param bb buffer to load from
	 */
	public ObjectRef(ByteBuffer bb) {
		dataAddr = bb.getInt();
		//interfaceAddr = bb.getInt();		
		//interfaceAddr = 0;		
	}

	public ObjectRef(int dataAddr, int ifAddr ) {
		this.dataAddr = dataAddr;
		//this.interfaceAddr = ifAddr;		
	}

	// TODO 64 bit problem
	public long getDataAddr()      {		return 0xFFFFFFFF & (long)dataAddr;	}
	//public long getInterfaceAddr() {		return 0xFFFFFFFF & (long)interfaceAddr;	}
	//public long getInterfaceAddr() {		return 0;	}

	//public ObjectHeader getObject() { return Main.getPhantomObject(dataAddr); }
	
	@Override
	public String toString() {
		return Long.toHexString(dataAddr);
	}


	// write itself to the byte buffer
	// based on constructor
	public void writeToByteBuffer(ByteBuffer byteBuffer){
		byteBuffer.putInt((int) this.dataAddr);
		//byteBuffer.putInt((int) this.interfaceAddr);
	}
}
