package ru.dz.phantom.code;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.Charset;

import ru.dz.plc.compiler.ConstantPool;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 * 
 * <p>Description: Write constant pool element to class file</p>
 * 
 * <p>Copyright: Copyright (c) 2016</p>
 * 
 * <p>Company: DZ Systems</p>
 * @author dz
 * @version 1.0
 */

public class ConstantPoolFileInfo extends FileInfo {

	
	private String sConst;
	private int ordinal;
	private PhantomType pt;

	public ConstantPoolFileInfo(RandomAccessFile os, int ordinal, String sConst )
	{
		super( os, (byte)'c' );
		this.ordinal = ordinal;
		this.sConst = sConst;
		
		pt = PhantomType.t_string; 
	}

	@Override
	protected void do_write_specific() throws IOException, PlcException 
	{
		Fileops.put_int32( os, ordinal );
		pt.save_to_file(os);
		//System.err.println("const type "+pt);
		byte[] bytes = sConst.getBytes(Charset.forName(ConstantPool.FILE_ENCODING));
		os.write(bytes); 
	}

}
