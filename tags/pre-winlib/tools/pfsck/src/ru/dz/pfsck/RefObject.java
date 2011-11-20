package ru.dz.pfsck;

//----------------------------------------------------------------------------------------
//	Copyright � 2006 - 2010 Tangible Software Solutions Inc.
//	This class can be used by anyone provided that the copyright notice remains intact.
//
//	This class is used to simulate the ability to pass arguments by reference in Java.
//----------------------------------------------------------------------------------------
public final class RefObject<T>
{
	public T argvalue;
	public RefObject(T refarg)
	{
		argvalue = refarg;
	}
}