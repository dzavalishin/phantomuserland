package ru.dz.phantom.file.pclass;

import java.io.RandomAccessFile;

public class GenericLoaderHandler 
{
	protected RandomAccessFile is;
	protected int size;
	protected AbstractClassInfoLoader loader;

	public GenericLoaderHandler( RandomAccessFile is, int size, AbstractClassInfoLoader loader )
	{
		this.is = is;
		this.size = size;
		this.loader = loader;
	};

}
