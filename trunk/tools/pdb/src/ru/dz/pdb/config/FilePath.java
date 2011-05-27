package ru.dz.pdb.config;

import java.io.File;
import java.util.ArrayList;

public class FilePath {

	ArrayList <File> path = new ArrayList<File>();

	public FilePath() {
	}
	
	public FilePath(String string) {
		setPath(string);
	}

	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		boolean first = true;
		for( File f : path )
		{
			if(!first) sb.append(File.pathSeparator);
			first = false;
			sb.append(f);
		}
		
		return sb.toString();
	}
	
	/*
	public ArrayList<File> getPath() {
		return path;
	}

	public void setPath(ArrayList<File> path) {
		this.path = path;
	}
	*/
	
	public String getPath() { return toString(); }
	public void setPath(String s)
	{
		ArrayList <File> p = new ArrayList<File>();
		String[] split = s.split(File.pathSeparator);
		for( String pe : split )
			p.add(new File(pe));
		
		path = p;
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
			System.out.println("FilePath.find() look in "+full);
			if(full.canRead())
				return full;
		}
		
		return null;
	}

	
	
}


