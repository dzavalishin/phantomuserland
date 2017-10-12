/**
 * 
 */
package ru.dz.plc.parser;

import java.util.ArrayDeque;

import ru.dz.plc.util.PlcException;

/**
 * @author dz
 *
 * Unlimited unget() to be able to parse not just LR1 grammar.
 *
 */
public class LexStack implements ILex {

	private boolean debug = false;
	
	private ILex parent;

	public LexStack(ILex parent) {
		this.parent = parent;		
	}
	
	private static int MAXDEPTH = 1024;

	// left part - rightmost item is current
	private ArrayDeque<LexStackItem> l = new ArrayDeque<LexStackItem>();
	// right part, what was ungot() (returned back)
	private ArrayDeque<LexStackItem> r = new ArrayDeque<LexStackItem>();

	
	private void grow_r() throws PlcException
	{
		if(l.size() <= 0)
			throw new PlcException("grow_r","can't unget - empty get list");

		LexStackItem i = l.removeLast();
		r.addFirst(i);		
	}
	
	private void grow_l() throws PlcException
	{
		LexStackItem i;
		
		if(r.size() > 0)
		{
			i = r.removeFirst();
		}
		else
		{
			if(l.size() +r.size() > MAXDEPTH )
				throw new PlcException("grow_l","too many tokens in queue");

			// TODO if seen eof, never call again?
			i = new LexStackItem(parent);
		}
		
		l.addLast(i);		
	}
	
	/*
	private void refill() throws PlcException
	{
		if(l.size() +r.size() > MAXDEPTH )
			throw new PlcException("too many tokens in queue");
		
		if(l.size() <= 0)
		{
			grow_l();
		}
	}*/

	private LexStackItem curr() throws PlcException
	{
		//refill();
		assert(!l.isEmpty());
		return l.peekLast();
	}
	
	// -------------------------------------------------------------------
	// Get/return token.
	// -------------------------------------------------------------------
	
	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#get()
	 */
	@Override
	public Token get() throws PlcException {
		grow_l();
		if(debug) System.err.println( "get "+curr().t );
		return curr().t;
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#unget()
	 */
	@Override
	public void unget() throws PlcException {
		grow_r();
		if(debug) System.err.println( "unget to "+curr().t );
	}
	
	
	
	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#commit()
	 */
	@Override
	public void commit() {
		// Clear our ability to unget, leave just 2 items
		// Why 2? One unget is allways possible and 1 is for any case :)
		
		while( l.size() > 2)
		{
			l.removeFirst();
		}
	}
	
	// -------------------------------------------------------------------
	// Mark stack
	// -------------------------------------------------------------------
	
	private ArrayDeque<Integer> marks = new ArrayDeque<Integer>(); 
	
	@Override
	public void mark() {
		if(debug) System.err.println( "mark @ "+l.size() );
		marks.push(l.size());		
	}
	
	@Override
	public void unmark() {
		marks.pop();
		if(debug) System.err.println( "unmark to @ "+l.size() );
		if(marks.isEmpty())
			commit();
	}
	
	@Override
	public void rewind() throws PlcException {
		int pos = marks.pop();
		if(debug) System.err.println( "rewind to @ "+l.size() );
		while(l.size() > pos)
			grow_r();
		
	}
	
	
	
	// -------------------------------------------------------------------
	// Current token info
	// -------------------------------------------------------------------

	
	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#get_track()
	 */
	@Override
	public String get_track() throws PlcException {
		return curr().track;
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#get_line_number()
	 */
	@Override
	public int get_line_number() throws PlcException {
		return curr().line_number;
	}


	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#is_eof()
	 */
	@Override
	public boolean is_eof() throws PlcException {
		return curr().is_eof;
	}

	
	// -------------------------------------------------------------------
	// Setup and info
	// -------------------------------------------------------------------
	
	
	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#create_keyword(java.lang.String)
	 */
	@Override
	public int create_keyword(String w) {
		return parent.create_keyword(w);
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#create_token(ru.dz.plc.parser.Token)
	 */
	@Override
	public void create_token(Token t) {
		parent.create_token(t);
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#getFilename()
	 */
	@Override
	public String getFilename() {		
		return parent.getFilename();
	}

}


class LexStackItem
{
	Token 		t;
	boolean 	is_eof;
	int 		line_number;
	String 		track;

	public LexStackItem(ILex parent) throws PlcException {
		 t = parent.get();
		 is_eof = parent.is_eof();
		 line_number = parent.get_line_number();
		 track = parent.get_track();
	}

}
