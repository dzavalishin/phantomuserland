package ru.dz.phantom.file.pclass;

import java.io.IOException;
import java.io.RandomAccessFile;

import ru.dz.phantom.code.Fileops;

public class PhantomTypeInfo
{
	boolean container;
	String mainClassName;
	String containedClassName;
	
	public PhantomTypeInfo(RandomAccessFile is) throws IOException
	{
		container = Fileops.get_int32(is) != 0;
		mainClassName = Fileops.get_string(is);
		containedClassName = Fileops.get_string(is);
	}
	
	public boolean isContainer() {		return container;	}
	public void setContainer(boolean container) {		this.container = container;	}

	public String getMainClassName() {		return mainClassName;	}
	public void setMainClassName(String mainClassName) {		this.mainClassName = mainClassName;	}

	public String getContainedClassName() {		return containedClassName;	}
	public void setContainedClassName(String containedClassName) {		this.containedClassName = containedClassName;	}

	
	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		
		sb.append( getMainClassName() );
		if(isContainer())
		{
			sb.append( "( " );
			sb.append( getContainedClassName() );
			sb.append( "[])" );
		}

		return sb.toString();
	}
}

