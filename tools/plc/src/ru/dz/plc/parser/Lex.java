package ru.dz.plc.parser;

import ru.dz.plc.util.*;

//import java.util.List;
import java.util.Collection;
import java.util.LinkedList;
import java.io.BufferedInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.util.Iterator;

/**
 * <p>Lexical analiser.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class Lex implements ILex {
	private Token for_unget = null;
	private Token current = null;
	private LinkedList<Token> tokens = new LinkedList<Token>();
	private BufferedInputStream is;
	private boolean flag_eof = false;

	private int line_number = 1;

	// ----------------------- reporting ----------------------------------

	private final int track_len = 128;
	private StringBuffer track = new StringBuffer(track_len);

	private void add_to_track( char c )
	{
		/*if( c == Character.LINE_SEPARATOR )
    {
      track.setLength(0);
      return;
    }*/

		if (track.length() >= track_len)
			track.deleteCharAt(0);
		track.append(c);

		while( track.length() > 0 && !Character.isWhitespace( track.charAt(0) ) )
			track.deleteCharAt(0);
	}

	private void cut_last_track_char()
	{
		track.setLength(track.length()-1);
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#get_track()
	 */
	@Override
	public String get_track() { return track.toString(); }
	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#get_line_number()
	 */
	@Override
	public int get_line_number() { return line_number;}


	// ----------------------- id ----------------------------------

	private static int next_id = 0;
	private final String fname;
	static public int get_id() {
		return ++next_id;
	}


	// ----------------------- construction ----------------------------------


	public Lex( Collection<Token> tokens, String fname ) {
		this.fname = fname;
		this.tokens.addAll(tokens);
	}

	public void set_input( InputStream is ) {
		this.is = new BufferedInputStream( is );
	}

	// ----------------------- processing ----------------------------------

	// read in and skip whitespace
	private void skip_ws() throws IOException {
		while(true) {
			is.mark(1);
			int c = is.read();
			if( c < 0 )
			{
				is.reset();
				return;
			}

			if( c == Character.LINE_SEPARATOR )
				line_number++;

			if( Character.isWhitespace((char)c) ) continue;
			is.reset();
			return;
		}
	}

	private void skip_to_eol() throws IOException {
		//StringBuffer comm = new StringBuffer();
		while(true) {
			is.mark(1);
			int c = is.read();
			if( c < 0 )
			{
				is.reset();
				return;
			}
			if( c == Character.LINE_SEPARATOR )
				line_number++;

			// BUG? Use Character.LINE_SEPARATOR too?
			if( c == '\n' ) { /*System.out.println("skip: <"+comm.toString()+">"); */ return; }
			//comm.append((char)c);
		}
	}

	// read in and skip / * * / style comments
	private boolean skip_long_comment() throws IOException {

		while( true )
		{
			int c1 = is.read();
			if (c1 < 0)
				return false;

			if( c1 == Character.LINE_SEPARATOR )
				line_number++;

			is.mark(1);

			int c2 = is.read();
			if (c2 < 0)
				return false;

			if ( ( (char) c1) == '*' && ( (char) c2) == '/')
				return true;

			is.reset();
		}

	}


	// read in and skip comments
	private boolean skip_comments() throws IOException {

		is.mark(2);

		int c1 = is.read();
		if( c1 < 0 )
		{
			is.reset();
			return false;
		}

		int c2 = is.read();
		if( c2 < 0 )
		{
			is.reset();
			return false;
		}

		if( ((char)c1) == '/' && ((char)c2) == '/' )
		{
			skip_to_eol();
			return true;
		}

		if( ((char)c1) == '/' && ((char)c2) == '*' )
		{
			return skip_long_comment();
		}

		is.reset();
		return false;
	}



	private Token some_full_match( String in ) throws PlcException {
		for( Iterator<Token> i = tokens.iterator(); i.hasNext() ; ) {
			Token t = i.next();
			if( t.is(in) ) return t;
		}
		return null;
	}

	private boolean some_part_match( String in ) {
		for( Iterator<Token> i = tokens.iterator(); i.hasNext() ; ) {
			Token t = i.next();
			if( t.like(in) ) return true;
		}
		return false;
	}


	private Token scan_in() throws PlcException, IOException {
		//Token t = null;
		if( is == null )
			throw new PlcException( "Lex", "no input", null );

		skip_ws();
		while( skip_comments() ) // repeat until no comments found
			skip_ws();              // skip whitespace between comments

		// EOF - for some reason does not work with gcj :(
		//if( is.available() == 0 ) { flag_eof = true; return new TokenEof(); }

		add_to_track(' ');

		StringBuffer curr = new StringBuffer(32);

		String cs = null;
		Token last_token = null;
		int length = 0;

		while(true) {
			is.mark(1);
			int c = is.read();
			if( c < 0 )
			{
				is.reset();
				break;
			}

			curr.append((char)c);
			add_to_track((char)c);

			cs = curr.toString();

			// at least something complete is found
			Token new_t = some_full_match( cs );
			if( null != new_t )
			{
				last_token = new_t;
				length++;
				continue;
			}

			if( cs.length() > 64000 ) break; // too much

			if( some_part_match(cs) )
			{
				length++;
				continue;
			}

			break;
		}

		is.reset(); // return one last byte
		cut_last_track_char();

		// check for EOF here
		if( curr.length() == 0 )
		{
			flag_eof = true;
			return new TokenEof();
		}

		if( last_token == null )
			throw new PlcException("Lex", "no valid Token found", curr.toString() );

		//Token out = last_token.emit(cs.substring(0,cs.length()-1));
		Token out = last_token.emit(cs.substring(0,length));

		//if( out != null ) System.out.print(">> "+out.toString()+" ");

		return out;
	}


	// ----------------------- interface ----------------------------------

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#get()
	 */
	@Override
	public Token get() throws PlcException {

		try {
			if (current == null)
				current = scan_in();
		} catch(IOException ioe) {
			throw new PlcException("Lex", "IO error", ioe.toString() );
		}

		for_unget = current;
		current = null;
		return for_unget;
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#unget()
	 */
	@Override
	public void unget() throws PlcException {
		if( for_unget == null )
			throw new PlcException("Can't unget", "Lex", null);
		current = for_unget;
		for_unget = null;
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#commit()
	 */
	@Override
	public void commit() {
		// Empty. Only LexStack needs it.		
	}
	
	
	@Override
	public void mark() throws PlcException {
		throw new PlcException("Lex.mark", "unimpl");		
	}

	@Override
	public void unmark() throws PlcException {
		throw new PlcException("Lex.unmark", "unimpl");		
	}

	@Override
	public void rewind() throws PlcException {
		throw new PlcException("Lex.rewind", "unimpl");		
	}
	
	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#is_eof()
	 */
	@Override
	public boolean is_eof() { return flag_eof; }

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#create_keyword(java.lang.String)
	 */
	@Override
	public int create_keyword(String w) {
		int id = get_id();
		tokens.add( new Keyword( id, w ));
		return id;
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#create_token(ru.dz.plc.parser.Token)
	 */
	@Override
	public void create_token( Token t ) {
		//int id = get_id();
		tokens.add( t );
		//return id;
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.parser.ILex#getFilename()
	 */
	@Override
	public String getFilename() {
		return fname;
	}

}
