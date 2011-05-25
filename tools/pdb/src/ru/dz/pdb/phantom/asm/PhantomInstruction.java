package ru.dz.pdb.phantom.asm;

public class PhantomInstruction {

	private final String name;
	private final int arg1;
	private final boolean hasArg1;
	private final byte[] sarg;
	private final int arg2;
	private final boolean hasArg2; 

	public PhantomInstruction(String name) {
		this.name = name;

		arg1 = 0;
		hasArg1 = false;
		this.arg2 = 0;
		hasArg2 = false;
		
		this.sarg = null;
	}

	public PhantomInstruction(String name, int arg) {
		this.name = name;

		this.arg1 = arg;
		hasArg1 = true;
		this.arg2 = 0;
		hasArg2 = false;

		this.sarg = null;
	}

	public PhantomInstruction(String name, byte[] sarg) {
		this.name = name;
		this.sarg = sarg;

		this.arg1 = 0;
		hasArg1 = false;
		this.arg2 = 0;
		hasArg2 = false;
	}

	public PhantomInstruction(String name, int a1, int a2) {
		this.name = name;
		
		this.arg1 = a1;
		hasArg1 = true;
		this.arg2 = a2;
		hasArg2 = true;
		
		this.sarg = null;
	}

	@Override
	public String toString() {
		return 
			name + 
			(hasArg1 ? " "+Integer.toString(arg1) : "") +
			(hasArg2 ? " "+Integer.toString(arg2) : "")
			;
	}
	
}
