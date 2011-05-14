package ru.dz.pdb.phantom;

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
	private int dataAddr;
	private int interfaceAddr;

	/**
	 * 
	 * @param bb buffer to load from
	 * @param addr start address of object
	 */
	public ObjectRef(ByteBuffer bb, int addr) {
		bb.position(addr);
		
		dataAddr = bb.getInt();
		interfaceAddr = bb.getInt();		
	}

	/**
	 * 
	 * @param bb buffer to load from
	 */
	public ObjectRef(ByteBuffer bb) {
		dataAddr = bb.getInt();
		interfaceAddr = bb.getInt();		
	}

	public ObjectRef(int dataAddr, int ifAddr ) {
		this.dataAddr = dataAddr;
		this.interfaceAddr = ifAddr;		
	}

	
	public int getDataAddr()      {		return dataAddr;	}
	public int getInterfaceAddr() {		return interfaceAddr;	}
	
}
