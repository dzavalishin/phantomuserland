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

	private BicBasicGrammar g;
	
	public Ast( String fname ) throws FileNotFoundException {

		l = new Lex(tokens, fname);

		FileInputStream  fis = new FileInputStream ( fname );
		l.set_input(fis);
		
		g = new BicBasicGrammar(l, fname);
		
	}

	public BicAny parse() throws PlcException {
		return g.parse();
	}

	
	
}
