package ru.dz.plc.parser;

import java.util.HashSet;
import java.util.Set;

import ru.dz.plc.compiler.PhantomClass;

/**
 * <p>Title: Phantom Language Compiler</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2005</p>
 * <p>Company: </p>
 * @author dz
 * @version 1.0
 */

public class ParserContext 
{
	private String   excerpt;
	private int line_no;
	private final String fname;

	public ParserContext(Lex l) 
	{
		this.fname = l.getFilename();
		excerpt = new String( l.get_track() );
		line_no = l.get_line_number();
	}

	public ParserContext(String fname, int line) 
	{
		this.fname = fname;
		line_no = line;	
	}
	
	public String get_position() { return fname+":"+Integer.toString(line_no)+": "; }
	public String get_context() { return excerpt; }

	public int getLineNumber() { return line_no;	}

	

	
}
