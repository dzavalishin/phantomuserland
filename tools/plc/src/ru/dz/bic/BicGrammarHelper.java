package ru.dz.bic;

import ru.dz.plc.parser.GenericGrammarHelper;
import ru.dz.plc.parser.ILex;

public class BicGrammarHelper extends GenericGrammarHelper {

	protected final int id_lt;
	protected final int id_gt;
	protected final int id_tree;
	protected final int id_chain;
	protected final int id_colon;
	
	public BicGrammarHelper(ILex l, String fname) 
	{
		super(l, fname);
		
		id_lt          = l.create_keyword("<");
		id_gt          = l.create_keyword(">");
		id_colon       = l.create_keyword(":");
		id_tree        = l.create_keyword("tree");
		id_chain       = l.create_keyword("chain");

		initGeneralIds();
		
	}

}
