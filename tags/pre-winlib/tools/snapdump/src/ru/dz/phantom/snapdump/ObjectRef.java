package ru.dz.phantom.snapdump;

import java.nio.MappedByteBuffer;

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
	// TODO hardcode? cmd line override?
	public final static int VM_POOL_START = (0x40000000 * 2);

	private int dataAddr;
	private int interfaceAddr;

	/**
	 * 
	 * @param bb buffer to load from
	 * @param addr start address of object
	 */
	public ObjectRef(MappedByteBuffer bb, int addr) {
		bb.position(addr);
		
		dataAddr = bb.getInt();
		interfaceAddr = bb.getInt();		
	}

	/**
	 * 
	 * @param bb buffer to load from
	 */
	public ObjectRef(MappedByteBuffer bb) {
		dataAddr = bb.getInt();
		interfaceAddr = bb.getInt();		
	}

	public ObjectRef(int dataAddr, int ifAddr ) {
		dataAddr = dataAddr;
		interfaceAddr = ifAddr;		
	}

	
	public static int convertAddress(int a )
	{
		if(a == 0)			return -1;
		// TO DO turn off!
		//if(a == 0x23232323)			return -1; // Crazy thing in VM pagefault fills not with null
		
		return a-VM_POOL_START;
	}
	
	public int getDataAddr()      {		return convertAddress(dataAddr);	}
	public int getInterfaceAddr() {		return convertAddress(interfaceAddr);	}
	
}
