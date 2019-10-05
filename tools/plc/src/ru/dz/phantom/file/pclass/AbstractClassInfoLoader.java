package ru.dz.phantom.file.pclass;

import java.io.IOException;
import java.io.RandomAccessFile;

import ru.dz.phantom.code.Fileops;
import ru.dz.plc.compiler.Method;
import ru.dz.plc.util.PlcException;

public abstract class AbstractClassInfoLoader {
	private boolean debugMode = false;

	protected RandomAccessFile is;
	protected String class_name, class_parent_name;

	public AbstractClassInfoLoader(RandomAccessFile is)
	{
		this.is = is;
	}
	
	
	public String get_class_name() { return class_name; }
	
	@SuppressWarnings("unused")
	public boolean load_class_file() throws IOException, PlcException {

		long fsize = is.length();
		//printf("fsize %d\n", fsize );

		//byte data[] = new byte[fsize];
		//is.read(data,0,fsize);

		// parse it
		long record_size = 0;
		boolean got_class_header = false;

		for( long rec_start = 0; rec_start < fsize; rec_start = rec_start+record_size )
		{
			if(isDebugMode()) System.out.print("\n");

			is.seek(rec_start);
			long ptr = rec_start;
			//printf("%d bytes left\n", contents + fsize - ptr );

			{
				byte mrk[] = new byte[5];
				//is.read(mrk,(int)rec_start,5);
				is.read(mrk);

				if(
						mrk[0] != 'p' ||
						mrk[1] != 'h' ||
						mrk[2] != 'f' ||
						mrk[3] != 'r' ||
						mrk[4] != ':' )
					throw new PlcException("import","No record marker in class file");
			}

			ptr += 5;

			char record_type;

			{
				ptr++;
				byte rt = is.readByte();
				record_type = (char)rt;
			}

			record_size = is.readInt();

			if(isDebugMode())
			{
				System.out.print("type '" + record_type + "', size ");
				System.out.print(record_size);
				System.out.print(": ");
			}

			if( record_size < 6+8 )
				throw new PlcException("import","Invalid record size");

			// start of record-specific info
			ptr = is.getFilePointer();

			switch( record_type )
			{
			case 'C': // class
			{
				if( isClassDefined() )
					throw new PlcException("import","more than one class in class file", is.toString() );

				GenericLoaderHandler h = new GenericLoaderHandler( is, (int)(record_size - (ptr-rec_start)), this );
				class_name = Fileops.get_string(is);
				if(isDebugMode()) System.out.println( "Class is: "+class_name+"\n" );

				int n_object_slots = Fileops.get_int32(is);
				//if(debug_print) System.out.println(", %d fileds", n_object_slots );
				int n_method_slots = Fileops.get_int32(is);
				//if(debug_print) System.out.println(", %d methods\n", n_method_slots );
				class_parent_name = Fileops.get_string(is);

				createClass();

				got_class_header = true;
			}
			break;

			case 'M': // Method
				loadMethod(record_size, rec_start, ptr);
			break;

			case 'S': // Method signature
			{
//				MethodSignatureLoaderHandler mh = new MethodSignatureLoaderHandler( is, (int)(record_size - (ptr-rec_start)), this);
				GenericMethodSignatureLoaderHandler msh = getMethodSignatureLoaderHandler( is, (int)(record_size - (ptr-rec_start)));
				
				//methods.push_back(mh);

				//System.out.println(mh.me);

				//my_class.addMethod(msh.getMe());
			}
			break;

			case 'f':
				// Field
			{
				String fieldName = Fileops.get_string(is);
				int fOrdinal = Fileops.get_int32(is);
				//PhantomType fType = new PhantomType(is);
				PhantomTypeInfo ti = new PhantomTypeInfo(is);

				createField(fieldName, fOrdinal, ti);				
				//System.err.println("Field "+fieldName+" @ "+fOrdinal+" : "+fType);				
			}
			break;

			case 'l':
				// Line numbers, just ignore
				break;

			case 'c':
				// Constant pool element
			{
				int cOrdinal = Fileops.get_int32(is);
				//PhantomType cType = new PhantomType(is);
				PhantomTypeInfo ti = new PhantomTypeInfo(is);

				long start = is.getFilePointer();
				long len = record_size - (start-ptr) - 10; // 10 is record header size

				// TODO magic 64 mbytes max const len
				if( (len < 0 ) || (len > 64*1024*1024))
					throw new PlcException("class load","const size = "+len);

				byte[] buf = new byte[(int)len];
				is.readFully(buf, 0, buf.length);

				createPoolConst(cOrdinal, ti, buf);
			}
			break;

			default:
				System.err.println("Record of unknown type "+record_type);
				break;

			}

		}

		if( !got_class_header )
			return false;

		return true;
	}


	protected void loadMethod(long record_size, long rec_start, long ptr) throws IOException {
		MethodLoaderHandler mh = new MethodLoaderHandler( is, (int)(record_size - (ptr-rec_start)), this );
		// Ignore, we load method in signature record
	}
	

	protected abstract GenericMethodSignatureLoaderHandler getMethodSignatureLoaderHandler(RandomAccessFile is2, int i) throws IOException;

	protected abstract boolean isClassDefined();

	protected abstract void createPoolConst(int cOrdinal, PhantomTypeInfo ti, byte[] buf) throws IOException;

	protected abstract void createField(String fieldName, int fOrdinal, PhantomTypeInfo ti) throws IOException;

	protected abstract void createClass() throws IOException;


	public void addMethod(Method me) {
		// TODO Auto-generated method stub
		
	}


	public boolean isDebugMode() { return debugMode;	}
	public void setDebugMode(boolean debugMode) {		this.debugMode = debugMode;	}

	
}
















class MethodLoaderHandler extends GenericLoaderHandler
{
	String    my_name;
	int       my_ordinal;
	//phantom_object my_code;

	public MethodLoaderHandler( RandomAccessFile is, int in_size, AbstractClassInfoLoader _loader ) throws
	IOException {
		super( is, in_size, _loader );

		my_name = Fileops.get_string(is);
		//if(debug_print) System.out.println("Method is: ", my_name );

		my_ordinal = Fileops.get_int32(is);
		//if(debug_print) System.out.println(", ordinal: ", my_ordinal, "\n" );
	}

	public int get_ordinal() { return my_ordinal; }
	public String get_name() { return my_name; }
};








