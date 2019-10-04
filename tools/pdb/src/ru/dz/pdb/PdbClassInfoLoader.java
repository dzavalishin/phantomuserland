package ru.dz.pdb;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

import ru.dz.pdb.phantom.asm.Disassembler;
import ru.dz.phantom.code.Fileops;
import ru.dz.phantom.file.pclass.AbstractClassInfoLoader;
import ru.dz.phantom.file.pclass.GenericMethodSignatureLoaderHandler;
import ru.dz.phantom.file.pclass.PhantomTypeInfo;
import ru.dz.plc.util.PlcException;

public class PdbClassInfoLoader extends AbstractClassInfoLoader {

	public PdbClassInfoLoader(RandomAccessFile is) {
		super(is);
	}

	@Override
	protected GenericMethodSignatureLoaderHandler 
	  getMethodSignatureLoaderHandler(RandomAccessFile is2, int i)
			throws IOException {
		return new PdbMethodSignatureLoaderHandler(is2, i, this);
	}

	@Override
	protected boolean isClassDefined() {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	protected void createPoolConst(int cOrdinal, PhantomTypeInfo ti, byte[] buf) throws IOException {
		// TODO Auto-generated method stub
		
	}

	@Override
	protected void createField(String fieldName, int fOrdinal, PhantomTypeInfo ti) throws IOException {
		// TODO Auto-generated method stub
		
	}

	@Override
	protected void createClass() throws IOException {
		// TODO Auto-generated method stub
		
	}


	@Override
	protected void loadMethod(long record_size, long rec_start, long ptr) throws IOException {
		int sz = (int)(record_size - (ptr-rec_start));

		String    my_name;
		int       my_ordinal;
		//phantom_object my_code;

		long start = is.getFilePointer();
		
		my_name = Fileops.get_string(is);
		System.out.println("Method is: " + my_name );

		my_ordinal = Fileops.get_int32(is);
		System.out.println(", ordinal: " + my_ordinal + "\n" );
		
		long endHeader = is.getFilePointer();

		// Now load code
		
		int codeLength = (int) (sz - (endHeader - start));
		
		System.out.println("\tCode len="+codeLength);
		
		assert(codeLength > 0);
		
		byte[] buf = new byte[codeLength];
		is.readFully(buf, 0, codeLength);
	
		new Disassembler(buf).dump();
			

	}
	
	
	public static void main(String[] args) throws IOException, PlcException 
	{
		String pcFileName = System.getenv("PHANTOM_HOME") + "/plib/bin/ru.dz.phantom.system.boot.pc";
		RandomAccessFile is;
		
		try {
			
			is = new RandomAccessFile(pcFileName, "r");
			PdbClassInfoLoader cl = new PdbClassInfoLoader(is);
			
			cl.setDebugMode(true);
			cl.load_class_file();

		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}
	
}





class PdbMethodSignatureLoaderHandler extends GenericMethodSignatureLoaderHandler
{
	//private Method    					me = null;
	//private LinkedList<ArgDefinition> 	args = new LinkedList<ArgDefinition>();

	public PdbMethodSignatureLoaderHandler(RandomAccessFile is, int in_size, AbstractClassInfoLoader _loader)
			throws IOException {
		super(is, in_size, _loader);
/*				
		
		long start = is.getFilePointer();
		
		load();
		
		long endHeader = is.getFilePointer();
		
		//_loader.addMethod(me);

		// Now load code
		
		int codeLength = (int) (in_size - (endHeader - start));
		
		System.out.println("\tSignature code len="+codeLength);
		
		assert(codeLength > 0);
		
		byte[] buf = new byte[codeLength];
		is.readFully(buf, 0, codeLength);
	
		new Disassembler(buf).dump();
*/	
	}


	@Override
	protected void createArg(String arg_name, PhantomTypeInfo arg_ti) throws IOException {
		System.out.println("\tArg "+arg_name);
		/*
		PhantomType arg_type;
		try {
			arg_type = new PhantomType(arg_ti);
			ArgDefinition ad = new ArgDefinition( arg_name, arg_type );

			args.add(ad);
			getMe().addArg(arg_name, arg_type);
		} catch (PlcException e) {
			throw new IOException(e);
		}
		*/
	}

	@Override
	protected void createMethod(boolean constructor, PhantomTypeInfo ret_ti) throws IOException {
		System.out.println("Method "+myName);

		/*
		try {
			PhantomType myReturnType = new PhantomType(ret_ti);

			me = new Method( myName, myReturnType, constructor );
			me.setOrdinal(myOrdinal);
			//return my_return_type;
		} catch (PlcException e) {
			throw new IOException(e);
		}
		 * 
		 */
	}

	public int getOrdinal() { return myOrdinal; }
	public int getArgCount() { return myArgCount; }
	public String getName() { return myName; }

	//public Method getMe() {		return me;	}



};


