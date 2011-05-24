package ru.dz.pdb.config;

import java.io.File;
import java.util.ArrayList;

public class FilePath {

	ArrayList <File> path = new ArrayList<File>();

	public FilePath(String string) {
		String[] split = string.split(File.pathSeparator);
		for( String pe : split )
			path.add(new File(pe));
	}

	public ArrayList<File> getPath() {
		return path;
	}

	public void setPath(ArrayList<File> path) {
		this.path = path;
	}
	
	public void add( File p )
	{
		path.add(p);
	}

	public File find(String shortName)
	{
		for( File prefix : path )
		{
			File full = new File( prefix, shortName );
			if(full.canRead())
				return full;
		}
		
		return null;
	}

	
	
}


