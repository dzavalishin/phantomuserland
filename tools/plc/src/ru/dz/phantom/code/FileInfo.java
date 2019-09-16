package ru.dz.phantom.code;

import java.io.*;

import ru.dz.plc.util.*;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 * 
 * <p>Description: This class is used to write a class file header record.
 * Each record has a tag (type). records with lowercase tags can be ignored
 * by loader, and capital letter tags have to be understood. </p>
 * 
 * <p>Copyright: Copyright (c) 2004-2009</p>
 * 
 * <p>Company: Digital Zone </p>
 * @author dz
 */

public abstract class FileInfo {
	protected RandomAccessFile os;
	private byte tag;
	private boolean written;
	private long pos, end;


	protected FileInfo( RandomAccessFile os, byte tag ) {
		this.os = os;
		this.tag = tag;

		written = false;
	}

	public void write() throws PlcException, IOException {
		if( written ) throw new PlcException( "FileInfo::write", "already written", toString() );
		pos = os.getFilePointer();
		written = true;
		do_write();
		do_write_specific();
		end = os.getFilePointer();
		os.seek(pos);
		do_write();
		os.seek(end);
	}

	/*public void reWrite() throws PlcException, IOException {
    if( !written ) throw new PlcException( "FileInfo::reWrite", "not written yet", toString() );
  }*/

	protected void do_write() throws IOException {
		os.writeBytes("phfr:");
		os.writeByte(tag);
		os.writeInt(myLength());
	}

	private int myLength()
	{
		long len = end-pos;
		return (int)len;
	}

	//public void setEnd() throws IOException { end = os.getFilePointer(); }


	// ------------------------------------------------------------


	protected abstract void do_write_specific() throws IOException, PlcException;

}

