package ru.dz.plc.parser;

import ru.dz.plc.util.PlcException;

public interface ILex {

	// -------------------------------------------------------------------
	// Get/return token.
	// -------------------------------------------------------------------
	
	/**
	 * Get next token.
	 * @return Token.
	 * @throws PlcException
	 */
	Token get() throws PlcException;

	/**
	 * Return token back. Depth depends on impl. LexStack permits a lot. Lex permits one.
	 * @throws PlcException
	 */
	void unget() throws PlcException;

	/**
	 * Push mark - remember place in token stream.
	 * @throws PlcException 
	 */
	void mark() throws PlcException;

	/**
	 * Pop mark - forget last place in token stream. If mark stack is empty - commit.
	 * @throws PlcException 
	 */
	void unmark() throws PlcException;

	/**
	 * Pop mark and unget to marked place.
	 * @throws PlcException 
	 */
	void rewind() throws PlcException;

	
	
	/**
	 *  Grammar is sure it will not do more than one unget after this and before next get.
	 */
	void commit();

	// -------------------------------------------------------------------
	// Current token info
	// -------------------------------------------------------------------
	
	String get_track() throws PlcException;

	int get_line_number() throws PlcException;


	boolean is_eof() throws PlcException;

	// -------------------------------------------------------------------
	// Setup and info
	// -------------------------------------------------------------------
	
	int create_keyword(String w);

	void create_token(Token t);
	
	String getFilename();


		
}