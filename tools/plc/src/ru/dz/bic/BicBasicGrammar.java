package ru.dz.bic;

import ru.dz.plc.parser.ILex;
import ru.dz.plc.parser.Token;
import ru.dz.plc.util.PlcException;

public class BicBasicGrammar extends BicGrammarHelper {

	public BicBasicGrammar( ILex l, String fname ) {
		super( l, fname );
	}

	
	public BicAny parse() throws PlcException {
		return parseAny();
	}
	
	private BicChain parseChain() throws PlcException {
		//expect( id_lt );
		expect( id_chain );
	
		BicChain chain = new BicChain();
		
		while(true)
		{
			if( testAndEat(id_gt) ) return chain;
			
			expect( id_lt );
			chain.add( parseTree() );
		}
	}


	private BicTree parseTree() throws PlcException {
		//expect( id_lt );
		expect( id_tree);

		String treeType = getIdent();
		
		BicTree ret = new BicTree(treeType);
		
		// Now parse children
		
		while(true)
		{
			if( testAndEat(id_gt) ) return ret;
			
			String prefix = parseTreeNodePrefix();
			
			ret.add( prefix, parseAny() );
		}
		
	}



	private BicAny parseAny() throws PlcException {
		int id = peek();

		if( id != id_lt )
		{			
			if( id == id_int_const ) return new BicIntConst( getInt() );
			if( id == id_string_const ) return new BicStringConst( getString() );
		}
		else
		{
			expect( id_lt );
		
			id = peek();
		
			if( id == id_chain ) return parseChain(); 
			if( id == id_tree ) return parseTree();
			//if( id == id_int_const ) return new BicIntConst( getInt() );
			//if( id == id_string_const ) return new BicStringConst( getString() );
		}
		
		Token t = l.get();
		
		syntax_error("unknown node class: "+t.value());
		throw new PlcException("parseAny", "id = " + Integer.toString(t.get_id()));
	}


	private String parseTreeNodePrefix() throws PlcException {
		
		Token nodeType = l.get();
		if( nodeType.get_id() != id_ident )
			syntax_error("Not an ident in parseTreeNodePrefix");

		expect( id_colon );
		
		return nodeType.value();
	}


	
}
