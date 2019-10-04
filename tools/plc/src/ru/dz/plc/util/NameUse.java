package ru.dz.plc.util;

/**
 * Tells if name is used as field, static field or method.
 * @author dz
 *
 */

public class NameUse 
{

	private boolean usedAsField;
	private boolean usedAsStaticField;
	private boolean usedAsMethod;
	
	public NameUse(boolean usedAsField, boolean usedAsStaticField, boolean usedAsMethod) {
		this.usedAsField = usedAsField;
		this.usedAsStaticField = usedAsStaticField;
		this.usedAsMethod = usedAsMethod;

	}

	public boolean isUsedAsField() { 		return usedAsField;	}
	public boolean isUsedAsStaticField() { 		return usedAsStaticField;	}
	public boolean isUsedAsMethod() {		return usedAsMethod;	}

	public boolean isUsed() { 		return usedAsField || usedAsStaticField || usedAsMethod;	}
	
}
