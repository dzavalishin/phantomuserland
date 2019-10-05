package ru.dz.pdb.phantom.asm;

import phantom.code.opcode_ids;

public class PhantomInstruction extends opcode_ids {

	private final String name;
	private final int arg1;
	private final boolean hasArg1;
	private final byte[] sarg;
	private final int arg2;
	private final boolean hasArg2;
	private byte prefix; 

	public PhantomInstruction(byte prefix, String name) {
		this.prefix = prefix;
		this.name = name;

		arg1 = 0;
		hasArg1 = false;
		this.arg2 = 0;
		hasArg2 = false;

		this.sarg = null;
	}

	public PhantomInstruction(byte prefix, String name, int arg) {
		this.prefix = prefix;
		this.name = name;

		this.arg1 = arg;
		hasArg1 = true;
		this.arg2 = 0;
		hasArg2 = false;

		this.sarg = null;
	}

	public PhantomInstruction(byte prefix, String name, byte[] sarg) {
		this.prefix = prefix;
		this.name = name;
		this.sarg = sarg;

		this.arg1 = 0;
		hasArg1 = false;
		this.arg2 = 0;
		hasArg2 = false;
	}

	public PhantomInstruction(byte prefix, String name, int a1, int a2) {
		this.prefix = prefix;
		this.name = name;

		this.arg1 = a1;
		hasArg1 = true;
		this.arg2 = a2;
		hasArg2 = true;

		this.sarg = null;
	}

	private String argToString(boolean has, int arg) {
		if(!has)
			return "";

		return String.format(" %d (0x%X)", arg, arg );
	}

	@Override
	public String toString() {
		return
				prefixString() +
				name + 
				argToString(hasArg1, arg1) +
				argToString(hasArg2, arg2) +
				( (sarg != null) ? " \"" + new String(sarg) + "\"" : "" )
				;
	}

	private String prefixString() {
		switch(prefix)
		{
		case opcode_prefix_long:	return "long:";
		case opcode_prefix_float:	return "float:";
		case opcode_prefix_double:	return "double:";
		}
		return "";
	}

}
