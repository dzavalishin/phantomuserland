package ru.dz.plc.util;

public class EmptyPlcException extends PlcException {
	private static final long serialVersionUID = -1743245252663903561L;

	public EmptyPlcException( String where, String param )
	{
		super(where, "nothing found", param );
	}

	public EmptyPlcException( String where )
	{
		super(where, "nothing found", null );
	}
}
