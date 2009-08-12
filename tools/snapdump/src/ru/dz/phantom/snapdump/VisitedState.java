package ru.dz.phantom.snapdump;

public class VisitedState {
	private boolean invalidMemory = false;
	private AllocHeader allocHeader;
	private ObjectHeader objectHeader;

	public boolean isInvalidMemory() {		return invalidMemory;	}
	public void setInvalidMemory(boolean invalidMemory) {		this.invalidMemory = invalidMemory;	}

	public void setAllocHeader(AllocHeader allocHeader) {		this.allocHeader = allocHeader;	}
	public AllocHeader getAllocHeader() {		return allocHeader;	}

	public void setObjectHeader(ObjectHeader objectHeader) {		this.objectHeader = objectHeader;	}
	public ObjectHeader getObjectHeader() {		return objectHeader;	}

}
