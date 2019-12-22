package ru.dz.bic;

import java.io.FileNotFoundException;

import ru.dz.plc.util.PlcException;

public class Main {

	public static void main(String[] args) throws PlcException, FileNotFoundException {
		//Ast a = new Ast("p:/phantomuserland/tools/plc/phantom_bic_test.ast");
		Ast a = new Ast("p:/phantomuserland/tools/plc/phantom_bic_simple.ast");
		
		BicAny n = a.parse();

		//n.foreach( el -> { System.out.println(el); });
		n.convertExpressions();
		n.dump();
	}

}
