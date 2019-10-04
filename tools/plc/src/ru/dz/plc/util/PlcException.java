package ru.dz.plc.util;

import java.io.IOException;

public class PlcException extends Throwable {
	private static final long serialVersionUID = 2243954417063793676L;
	
	//String what = null, where = null, param = null;

	public PlcException( String where, String what, String param) {
		super(what + " in " + where + " (" + param + ")");
		//this.what = what; this.where = where; this.param = param;
	}

	public PlcException( String where, String what) {
		super(what + " in " + where);
		//this.what = what; this.where = where; this.param = null;
	}

	public PlcException( String where ) {
		super(where);
		//this.what = "(unknown)"; this.where = where; this.param = null;
	}


	public PlcException(String message, IOException cause) {
		super(message,cause);
	}

	/*
	public String toString() {
		String sub = what + " in " + where;
		return param == null ? sub : sub + " (" + param + ")";
	}*/
}

