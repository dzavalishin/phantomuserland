package ru.dz.pdb.phantom;

import java.util.ArrayList;

public abstract class AbstractProgramSource implements IProgramSource 
{
	ArrayList<String> lines = new ArrayList<String>(128); 
	
	@Override
	public String getLine(int lineNo) {
		return lines.get(lineNo);
	}

	@Override
	public int getLinesCount() {
		return lines.size();
	}
	
	@Override
	public void dump() 
	{
		int i = 0;
		for( String l : lines )
		{
			System.out.print(String.format("%4d: ", i++));
			System.out.println(l);
		}
		
	}
	
}
