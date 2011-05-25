package ru.dz.pdb.phantom;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;

import ru.dz.pdb.Main;

public class FileProgramSource extends AbstractProgramSource 
{
	
	public FileProgramSource(String className) throws IOException, PhantomClassNotFoundException 
	{
		// Cut off leading point
		if( className.startsWith(".") )
			className = className.substring(1);
		
		if( !className.endsWith(".ph") )
			className = className + ".ph";
		
		File fn = Main.getProject().findSourceFile(className);
		//File fn = new File(className);
		
		if(fn == null)
			throw new PhantomClassNotFoundException();
		
		FileInputStream fstream = new FileInputStream(fn);
	    DataInputStream in = new DataInputStream(fstream);
		
	    BufferedReader br = new BufferedReader(new InputStreamReader(in));
		
	    String strLine;

	    while ((strLine = br.readLine()) != null)   {
	      lines.add(strLine);
	    }
	    
	    in.close();	    	    
	}

	public static void main(String[] args) throws PhantomClassNotFoundException {
		try {
			FileProgramSource src = new FileProgramSource(".internal.binary");
			src.dump();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	
}
