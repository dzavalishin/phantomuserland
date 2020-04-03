package ru.dz.bic;

import java.io.File;
import java.io.FileNotFoundException;

import ru.dz.plc.PlcMain;
import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.util.PlcException;

public class Main {

	public static void main(String[] args) throws PlcException, FileNotFoundException 
	{
		// need basic classes such as int
		PlcMain.addClassFileSearchParh( new File("p:/phantomuserland/plib/bin") ); // TODO temp hack
		
		//Ast a = new Ast("p:/phantomuserland/tools/plc/phantom_bic_test.ast");
		Ast a = new Ast("p:/phantomuserland/tools/plc/phantom_bic_simple.ast");
		
		BicAny n = a.parse();

		ClassMap classes = ClassMap.get_map();
		// It is a default base class so we need it in any case.
		classes.do_import(".internal.object");

		classes.do_import(".internal.int");
		classes.do_import(".internal.long");
		classes.do_import(".internal.float");
		classes.do_import(".internal.double");
		classes.do_import(".internal.string");
		
		PhantomClass pc = new PhantomClass("c_program");
		
		ParseState ps = new ParseState(pc);
		
		n.foreach( el -> { el.setParseState(ps); });
		
		//n.foreach( el -> { System.out.println(el); });
		n.preprocess();
		n.convertExpressions();
		n.dump();
	}

}
