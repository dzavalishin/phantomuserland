package ru.dz.pdb.phantom;

import java.util.ArrayList;

public abstract class AbstractProgramSource implements IProgramSource 
{
	ArrayList<String> lines = new ArrayList<String>(128); 
	
	@Override
	public String getLine(int lineNo) {
		return lines.get(lineNo);
	}

}
