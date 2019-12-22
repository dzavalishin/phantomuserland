package ru.dz.bic;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.LinkedList;

import ru.dz.plc.parser.Lex;
import ru.dz.plc.parser.Token;
import ru.dz.plc.util.PlcException;

public class Ast {

	private Lex l;
	private static LinkedList<Token> tokens = new LinkedList<Token>();

	/*
	private final int id_ident;
	private final int id_string_const;
	private final int id_int_const;
	*/
	private BicBasicGrammar g;
	
	public Ast( String fname ) throws FileNotFoundException {

		l = new Lex(tokens, fname);

		FileInputStream  fis = new FileInputStream ( fname );
		l.set_input(fis);
		
		g = new BicBasicGrammar(l, fname);

		/* / parametric
		id_ident = Lex.get_id();
		l.create_token(new TokenIdent(id_ident));

		id_string_const = Lex.get_id();
		l.create_token(new TokenStringConst(id_string_const));

		id_int_const = Lex.get_id();
		l.create_token(new TokenIntConst(id_int_const));
		*/
		
		
	}

	public BicAny parse() throws PlcException {
		return g.parse();
	}

	
	
}
