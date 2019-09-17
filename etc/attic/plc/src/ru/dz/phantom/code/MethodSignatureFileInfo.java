package ru.dz.phantom.code;

import java.io.*;
import ru.dz.plc.compiler.*;
import ru.dz.plc.util.EmptyPlcException;
import ru.dz.plc.util.PlcException;

import java.util.*;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: </p>
 * @author dz
 * @version 1.0
 */

public class MethodSignatureFileInfo extends FileInfo {
  Method m;
  CodeGeneratorState s;

  public MethodSignatureFileInfo(RandomAccessFile os, Method m, CodeGeneratorState s )
  {
    super( os, (byte)'S' );
    this.m = m;
    this.s = s;
  }

  protected void do_write_specific() throws IOException, PlcException,
      EmptyPlcException {
    Fileops.put_string_bin( os, m.name );
    Fileops.put_int32( os, m.ordinal );
    Fileops.put_int32( os, m.args_def.size() );

    m.type.save_to_file(os);

    for( Iterator<ArgDefinition> i = m.args_def.iterator(); i.hasNext(); )
    {
      ArgDefinition ad = i.next();

      String arg_name = ad.getName();
      PhantomType arg_type = ad.getType();

      Fileops.put_string_bin( os, arg_name );
      arg_type.save_to_file(os);
    }

  }

}

