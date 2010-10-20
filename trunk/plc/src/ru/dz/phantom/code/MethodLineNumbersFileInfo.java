package ru.dz.phantom.code;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.Map;

import ru.dz.plc.compiler.Method;
import ru.dz.plc.util.PlcException;

public class MethodLineNumbersFileInfo extends FileInfo {

	private final Method m;

	public MethodLineNumbersFileInfo(RandomAccessFile os, Method m) {
		super(os, (byte)'l');
		this.m = m;
	}

	@Override
	protected void do_write_specific() throws IOException, PlcException {
		Codegen cg = m.get_cg();

		Map<Long, Integer> map = cg.getIpToLine();
		
		// Method ordinal
		Fileops.put_int32( os, m.getOrdinal() );
		// Num of entries
		Fileops.put_int32( os, map.size() );
		//System.out.println("MethodLineNumbersFileInfo nentries = "+map.size());
		
		// Each entry
		for( Long ip : map.keySet() )
		{
			int line = map.get(ip);
			Fileops.put_int64( os, ip );
			Fileops.put_int32( os, line );
			//System.out.println("ip "+ip+" -> line "+line);
		}
		
	}

	
	
}
