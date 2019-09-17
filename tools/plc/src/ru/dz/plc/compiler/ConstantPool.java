package ru.dz.plc.compiler;

import java.io.IOException;
import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.Map;

import ru.dz.phantom.code.Codegen;
import ru.dz.phantom.code.ConstantPoolFileInfo;
import ru.dz.plc.util.PlcException;

/**
 * <p>Pool of constants for a class.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2016 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class ConstantPool {
	public static final String FILE_ENCODING = "UTF-8";
	private Map<Integer, Object> table;
	private ordinals_generator ordinals = new ordinals_generator();

	public ConstantPool() {
		table = new HashMap<Integer, Object>();
	}
	
	/**
	 * Add a string constant to a pool.
	 * 
	 * TODO - identical strings must have one id
	 * 
	 * @param constant Constant to put.
	 * @return id - number of constant slot, used in const_pool instruction.
	 * 
	 * @see Codegen.emitConstantPool
	 */
	int add( String constant )
	{
		int id = ordinals.getNext();
		
		table.put(id, constant);
		
		return id;
	}
	
	
//	public void codegen(RandomAccessFile os, FileWriter lst,
//			BufferedWriter llvmFile, BufferedWriter c_File, CodeGeneratorState s, String version) throws PlcException 
	public void codegen(CodeWriters cw) throws PlcException
	{
		//llvmFile.write("; fields: \n");
		for( int id : table.keySet())
		{
			Object v = table.get(id);
			
			boolean done = false;
			
			if (v instanceof String) 
			{
				String sv = (String) v;
				done = true;

				ConstantPoolFileInfo info = new ConstantPoolFileInfo(cw.get_os(), id, sv);
				
				try {
					info.write();
					cw.llvmFile.write("; label const_pool"+id+":\n; .string '"+sv+"'\n");
					cw.llvmFile.write("; - constant for const pool id "+id+" val '"+sv+"'\n");
					cw.c_File.write("static const char* const_pool"+id+" = \""+sv+"\"\n");
					//c_File.write("// - constant for const pool id "+id+" val '"+sv+"'\n");
				} catch (IOException e) {
					throw new PlcException("Writing const id "+id, e.toString());
				}		
			}
			
			if(!done)
				throw new PlcException("Unknown type writing const id "+id+" val='"+v.toString()+"'");
			
		}
	}

	public void setConstant(int cOrdinal, PhantomType cType, byte[] buf) throws PlcException {
		if( !cType.is_string() )
			System.err.println("ConstPool setConst: not string but "+cType);
		
		String s = new String( buf, Charset.forName(FILE_ENCODING));

		if( table.get(cOrdinal) != null )
			throw new PlcException("ConstPool setConst"," ordinal "+cOrdinal+" is used");
		
		table.put(cOrdinal, s);
	}


	
}
