package ru.dz.plc.parser;

import ru.dz.plc.util.PlcException;

/**
 * 
 * Does not depend on specific grammar. 
 * 
 * Used in BIC parser too.
 * 
 * @author dz
 *
 */

public class GenericGrammarHelper 
{
	
	protected int id_ident;
	protected int id_string_const;
	protected int id_int_const;

	public GenericGrammarHelper( ILex l, String fname ) {
		this.l = l;
		this.fname = fname;
		
	}
	
	/**
	 * Must be called from child c'tor after all keywords are defined.
	 * Lexer looks for tokens sequentially, generics must be last. 
	 */
	protected void initGeneralIds() 
	{
		id_ident = Lex.get_id();
		l.create_token(new TokenIdent(id_ident));

		id_string_const = Lex.get_id();
		l.create_token(new TokenStringConst(id_string_const));

		id_int_const = Lex.get_id();
		l.create_token(new TokenIntConst(id_int_const));
	}

	
	private int warning_count = 0;
	protected String fname = "unknown";

	public int get_warning_count() {
		return warning_count;
	}

	private int error_count = 0;

	public int get_error_count() {
		return error_count;
	}

	protected ILex l;
	

	
	/**
	 * syntax_warning
	 * 
	 * @param string
	 *            String
	 */
	protected void syntax_warning(String warning_text) throws PlcException {
		System.err.flush();

		// System.out.println("At '"+l.get_track()+"', line "+ (new
		// Integer(l.get_line_number())).toString() );
		System.out.print(fname + ":"
				+ Integer.toString(l.get_line_number()) + ": warning: " + warning_text);
		System.out.println();
		warning_count++;
		if (warning_count > 20) {
			throw new PlcException("Grammar", "too many warnings");
		}
	}

	protected void syntax_error(String err_text) throws PlcException {
		System.err.flush();

		System.out.print(fname+":"
				+ Integer.toString(l.get_line_number()) + ": error: " + err_text);
		
		System.out.println();
		
		error_count++;
		
		if (warning_count > 20) {
			throw new PlcException("Grammar", "too many errors");
		}
	}
	

	
	
	
	/**
	 * Consume and return expected token. If next token is not of expected kind,
	 * produce an error.
	 * 
	 * @param id
	 *            Type of token to expect.
	 * @return Token of expected type or null if next token is of other type.
	 * @throws PlcException
	 */
	protected Token expect(int id) throws PlcException {
		Token nt = l.get();
		if (nt.get_id() == id)
			return nt;
		l.unget();
		syntax_error("Unexpected " + nt.toString());
		return null;
	}

	/**
	 * Consume and return expected token. If next token is not of expected kind,
	 * produce an error.
	 * 
	 * @param id
	 *            Type of token to expect.
	 * @param err
	 *            Error message to produce if next token is not of expected
	 *            type.
	 * @return Token of expected type or null if next token is of other type.
	 * @throws PlcException
	 */
	protected Token expect(int id, String err) throws PlcException {
		Token nt = l.get();
		if (nt.get_id() == id)
			return nt;
		l.unget();
		syntax_error("Near " + nt.toString() + ": " + err);
		return null;
	}

	/**
	 * Check if the next token is of given kind and if so - just consume it and
	 * return true. Else do not consume token and return false.
	 * 
	 * @param id
	 *            Type token to check for.
	 * @return True if token of given type was consumed.
	 * @throws PlcException
	 */
	protected boolean testAndEat(int id) throws PlcException {
		Token nt = l.get();
		if (nt.get_id() == id)
			return true;
		l.unget();
		return false;
	}

	protected boolean peek(int id) throws PlcException {
		Token nt = l.get();
		l.unget();
		return nt.get_id() == id;
	}

	/**
	 * Take a look at the next token, but not consume it.
	 * @return token id 
	 * @throws PlcException
	 */
	protected int peek() throws PlcException {
		Token nt = l.get();
		l.unget();
		return nt.get_id();
	}

	

	
	
	/**
	 * Parse integer or produce an error.
	 * 
	 * @param id_int_const
	 *            integer constant token id
	 * @return integer parsed.
	 */
	protected int getInt(int id_int_const) throws PlcException {
		Token nt = l.get();
		if (nt.get_id() == id_int_const) {
			TokenIntConstVal iv = (TokenIntConstVal) nt;
			return iv.int_value();
		} else {
			l.unget();
			syntax_error("not an integer " + nt.toString());
		}
		return 0;
	}

	protected int getInt() throws PlcException {
		return getInt(id_int_const);
	}
	
	
	protected String getString(int id_string_const) throws PlcException {
		Token nt = l.get();
		if (nt.get_id() == id_string_const)
			return nt.value();
		else {
			l.unget();
			syntax_error("not a string " + nt.toString());
		}
		return "(error)";
	}

	protected String getString() throws PlcException {
		return getString(id_string_const);
	}
	
	protected String getIdent(int id_ident) throws PlcException {
		Token nt = l.get();
		if (nt.get_id() == id_ident)
			return nt.value();
		else {
			l.unget();
			syntax_error("not an identifier " + nt.toString());
		}
		return "(error)";
	}
	
	protected String getIdent() throws PlcException {
		return getIdent(id_ident);
	}	
}
