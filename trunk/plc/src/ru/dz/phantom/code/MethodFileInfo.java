package ru.dz.phantom.code;

import java.io.*;

import ru.dz.plc.compiler.*;
import ru.dz.plc.util.EmptyPlcException;
import ru.dz.plc.util.PlcException;

/**
 * <p>Method accessor.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class MethodFileInfo extends FileInfo {

    Method m;
    CodeGeneratorState s;
	private final FileWriter lst;

    public MethodFileInfo(RandomAccessFile os, FileWriter lst, Method m, CodeGeneratorState s ) {
    	super( os, (byte)'M' );
      this.lst = lst;
      this.m = m;
      this.s = s;
    }

    protected void do_write_specific() throws IOException, PlcException,
      EmptyPlcException {
      Fileops.put_string_bin( os, m.name );
      Fileops.put_int32( os, m.ordinal );
      //Codegen c = new Codegen( os );
      m.get_cg().set_os(os, lst);
      //if(m.code != null) m.code.generate_code ( c, s );
      //else c.emit_ret(); // empty function code
      m.generate_code( s );
      long end = os.getFilePointer();
      m.get_cg().relocate_all(); // it will seek to the last relocation point
      os.seek(end);
    }



}
