package ru.dz.pdb.phantom;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;

public class FileProgramSource extends AbstractProgramSource 
{
	
	public FileProgramSource(String className) throws IOException 
	{
		// Cut off leading point
		if( className.startsWith(".") )
			className = className.substring(1);
		
		File fn = new File(className);
		
		FileInputStream fstream = new FileInputStream(fn);
	    DataInputStream in = new DataInputStream(fstream);
		
	    BufferedReader br = new BufferedReader(new InputStreamReader(in));
		
	    String strLine;

	    while ((strLine = br.readLine()) != null)   {
	      lines.add(strLine);
	    }
	    
	    in.close();	    	    
	}
	
}
