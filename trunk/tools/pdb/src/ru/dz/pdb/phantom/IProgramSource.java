package ru.dz.pdb.phantom;

public interface IProgramSource 
{
	String getLine(int lineNo);
	int getLinesCount();
	void dump();
}
