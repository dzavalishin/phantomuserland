package ru.dz.plc.compiler;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintStream;
import java.io.RandomAccessFile;
import java.util.Calendar;
import java.util.TimeZone;

import ru.dz.plc.PlcMain;

/**
 * 
 * <p>Compiler outputs file io</p>
 *   
 * <p>Place to keep all the output file writers for all formats we generate:
 * <ul>
 * <li>Phantom class file (pc)
 * <li>listings (lst, lstc)
 * <li>sources in C and LLVM
 * <li>Java stubs
 * </ul></p>
 * 
 * @author dz
 *
 */
public class CodeWriters {
	private RandomAccessFile	os;
	
	//FileWriter				lstc;
	BufferedWriter				lstc;
	BufferedWriter				llvmFile;
	BufferedWriter 				c_File;
	BufferedWriter				javaFile;
	
	private FileOutputStream    lstos;
	private PrintStream 		lstps;

	private FileOutputStream 	d_os;
	private PrintStream 		d_ps;
	
	private String verstr; 
	
	public CodeWriters(String name) throws IOException {
		//SootMain.say("gen class "+c);

		// TODO name = name.substring(1)// skip leading point
		
		String classfns	= name+".pc";
		String lstcfns	= name+".lstc";
		String llvmfns	= name+".ll";
		String c_fns	= name+".c";
		String lstfns 	= name+".lst";
		String d_fns 	= name+".d";
		String java_fns 	= name+".java";

		// skip leading point
		File classfn = new File( PlcMain.getOutputPath(), classfns.substring(1));
		File lstcfn = new File( PlcMain.getOutputPath(), lstcfns.substring(1));
		File llvmfn = new File( PlcMain.getOutputPath(), llvmfns.substring(1));
		File c_fn = new File( PlcMain.getOutputPath(), c_fns.substring(1));
		File lstfn = new File( PlcMain.getOutputPath(), lstfns.substring(1));			
		File d_fn = new File( PlcMain.getOutputPath(), d_fns.substring(1));			
		File java_fn = new File( PlcMain.getOutputPath(), java_fns.substring(1));			

		classfn.delete();
		os = new RandomAccessFile( classfn, "rw" );

		lstcfn.delete();
		// TODO lstc = new BufferedWriter( new FileWriter(lstcfn) );
		//lstc = new FileWriter(lstcfn);
		lstc = new BufferedWriter( new FileWriter(lstcfn) );

		llvmfn.delete();
		llvmFile = new BufferedWriter( new FileWriter(llvmfn) );

		c_fn.delete();
		c_File = new BufferedWriter( new FileWriter(c_fn) );

		java_fn.delete();
		javaFile = new BufferedWriter( new FileWriter(java_fn) );

		lstfn.delete();	
		lstos = new FileOutputStream( lstfn );
		//BufferedOutputStream bos = new BufferedOutputStream(os);
		lstps = new PrintStream(lstos);

		d_fn.delete();
		d_os = new FileOutputStream( d_fn );
		//BufferedOutputStream bos = new BufferedOutputStream(os);
		d_ps = new PrintStream(d_os);
		
		{
			/*
				File verfn = new File( PlcMain.getOutputPath(), verfns.substring(1));
				RandomAccessFile vf = new RandomAccessFile( verfn, "rw" );
				String verstr = vf.readLine();
				int ver = 0;
				if(verstr.matches("Version=[0-9]+"))
				{

				}
				else
				{
					verstr = String.format("%4d", year, month, day, hour, min, sec );
				}*/


		}

		Calendar cc = Calendar.getInstance(TimeZone.getTimeZone("GMT"));						
		verstr = String.format("%4d%02d%02d%02d%02d%02d%03d", 
				cc.get(Calendar.YEAR), 
				cc.get(Calendar.MONTH), 
				cc.get(Calendar.DAY_OF_MONTH), 
				cc.get(Calendar.HOUR), 
				cc.get(Calendar.MINUTE), 
				cc.get(Calendar.SECOND),
				cc.get(Calendar.MILLISECOND)
				);
	}

	public void closeAll() throws IOException {
		os.close();
		lstc.close();
		llvmFile.close();
		c_File.close();
		javaFile.close();
		
		lstos.close();
		lstps.close();

		d_os.close();
		d_ps.close();
	}

	public RandomAccessFile get_of() {
		return os;
	}

	public String getVersionString() {
		return verstr;
	}

	public PrintStream get_lstps() {
		return lstps;
	}

	public PrintStream get_d_ps() {
		return d_ps;
	}

	public RandomAccessFile get_os() {
		return os;
	}

}
