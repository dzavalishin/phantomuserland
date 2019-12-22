package ru.dz.bic;

import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.node.IntConstNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.StatementsNode;
import ru.dz.plc.parser.GenericGrammarHelper;
import ru.dz.plc.parser.ILex;
import ru.dz.plc.parser.Token;
import ru.dz.plc.util.PlcException;

public class BicGrammar extends BicGrammarHelper {

	public BicGrammar( ILex l, String fname ) {
		super( l, fname );
	}

	
	public Node parse() throws PlcException {
		return parseTopLevelChain();
	}
	
	private Node parseTopLevelChain() throws PlcException {
		expect( id_lt );
		expect( id_chain );
	
		StatementsNode node = new StatementsNode();
		
		while(true)
		{
			if( testAndEat(id_gt) ) return node;
			
			node.addNode( parseTopLevelTree() );
		}
	}


	private Node parseTopLevelTree() throws PlcException {
		Node ret = null;
		expect( id_lt );
		expect( id_tree);

		//Token treeType = l.get();
		//if( treeType.get_id() != id_ident )
		//	syntax_error("Not an ident");
		String treeType = getIdent();
		
		// Now parse children
		
		switch(treeType)
		{
		case "T_FN_DEF": ret = parseFuncDef(); break;
		case "T_DECL": ret = parseDeclaration(); break;
		
		default:
			syntax_error("Unknown tree type: "+treeType ); 
			break;
		}
		
		expect( id_gt );
		return ret;
	}


	private Node parseFuncDef() throws PlcException {
		// We are inside tree, parse function definition sequence
		//String prefix;		
		//prefix = parseTreeNodePrefix();
		//if( prefix != "tFNDEF_STMTS") throw new PlcException("func def", "no statements");
		expectTreeNodePrefix( "func def", "tFNDEF_STMTS" ); 
		Node statements = parseStatementsChain(); 

		//prefix = parseTreeNodePrefix();
		//if( prefix != "tFNDEF_ARGS") throw new PlcException("func def", "no args");
		expectTreeNodePrefix( "func def", "tFNDEF_ARGS" );
		Node args = parseArgsChain(); 
		
		//prefix = parseTreeNodePrefix();
		//if( prefix != "tFNDEF_NAME") throw new PlcException("func def", "no name");
		expectTreeNodePrefix( "func def", "tFNDEF_NAME" );
		Node name = parseIdentifierTree(); 
		
		expectTreeNodePrefix( "func def", "tFNDEF_RET_TYPE" );
		Node type = parseTypeTree(); 
		
		return null;
	}


	
	private Node parseTypeTree() {
		// TODO Auto-generated method stub
		return null;
	}


	private Node parseIdentifierTree() {
		// TODO Auto-generated method stub
		return null;
	}


	private Node parseArgsChain() {
		// TODO Auto-generated method stub
		return null;
	}


	private Node parseStatementsChain() throws PlcException 
	{
		expect( id_lt );
		expect( id_chain );
	
		StatementsNode node = new StatementsNode();
		
		while(true)
		{
			if( testAndEat(id_gt) ) return node;
			
			node.addNode( parseStatement() );
		}
	}

	
	private Node parseStatement() throws PlcException {
		Node ret = null;
		expect( id_lt );
		expect( id_tree);

		String treeType = getIdent();
		
		// Now parse children
		
		switch(treeType)
		{
		//case "T_FN_DEF": ret = parseFuncDef(); break;
		case "T_DECL": ret = parseDeclaration(); break;
		case "T_RETURN": ret = parseReturn(); break;
		
		default:
			syntax_error("ParseStatement: Unknown tree type: "+treeType ); 
			break;
		}
		
		expect( id_gt );
		return ret;
	}

	
	
	
	
	

	private Node parseReturn() throws PlcException {
		expectTreeNodePrefix( "return", "tRET_EXP" );
		return parseExpression();
	}


	private Node parseExpression() throws PlcException {
		Node ret = null;
		expect( id_lt );
		expect( id_tree);

		String treeType = getIdent();
		
		switch(treeType)
		{
		case "T_INTEGER": 
			expectTreeNodePrefix(treeType, "tINT_VAL");
			ret = new IntConstNode( getInt() );
			break;

			// TODO write me
		case "T_ASSIGN": 
			expectTreeNodePrefix(treeType, "tASSIGN_RHS");
			Node rvalue = parseExpression();
			expectTreeNodePrefix(treeType, "tASSIGN_LHS");
			Node lvalue = parseExpression();
			ret = null;
			break;
		case "T_ADD": 
		case "T_SUB": 
			break;
		
		default:
			syntax_error("ParseStatement: Unknown tree type: "+treeType ); 
			break;
		}
		
		expect( id_gt );
		return ret;
	}

	
	
	
	

	private Node parseDeclaration() throws PlcException {
		// We are inside tree, parse declaration sequence

		expectTreeNodePrefix( "func def", "tDECL_OFFSET" );
		int decl_offset = getInt();

		expectTreeNodePrefix( "func def", "tDECL_TYPE" );
		Node type = parseDeclTypeTree();
				
		expectTreeNodePrefix( "func def", "tDECL_DECLS" );
		Node Decls = parseDeclDeclsTree();

		return null; // TODO 
	}
	
	
	
	
	private Node parseDeclDeclsTree() {
		// TODO Auto-generated method stub
		return null;
	}


	private Node parseDeclTypeTree() {
		// TODO Auto-generated method stub
		return null;
	}

	
	// ------------------------------------------------------------------------
	// Helpers
	// ------------------------------------------------------------------------
	
	

	void expectTreeNodePrefix(String where, String tag) throws PlcException
	{
		String prefix = parseTreeNodePrefix();
		if( prefix != tag) throw new PlcException( where, "no "+tag);
	}

	private String parseTreeNodePrefix() throws PlcException {
		
		Token nodeType = l.get();
		if( nodeType.get_id() != id_ident )
			syntax_error("Not an ident in parseTreeNodePrefix");

		expect( id_colon );
		
		return nodeType.value();
	}


	
}
