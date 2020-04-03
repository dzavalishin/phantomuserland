package ru.dz.bic;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.function.Consumer;

import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.binode.CallArgNode;
import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.binode.OpPlusNode;
import ru.dz.plc.compiler.node.ArgDefinitionNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.ReturnNode;
import ru.dz.plc.compiler.node.StatementsNode;
import ru.dz.plc.util.PlcException;

/**
 * 
 * Corresponds to AST file <tree ...>
 * 
 * @author dz
 *
 */
public class BicTree extends BicAny {

	private String treeType;


	public BicTree(String treeType) {
		this.treeType = treeType;
		
	}

	protected Map<String,BicAny> children = new HashMap<String,BicAny>(); 
	
	public void add(String prefix, BicAny any) throws PlcException {
		if( children.get(prefix) != null ) throw new PlcException("BicTree.Add", "dup");
		children.put(prefix, any);
	}
	
	@Override
	void foreach(Consumer<? super BicAny> action) {
		//children.values().forEach(action);
		children.values().forEach( child -> { child.foreach(action); } );
		action.accept(this);
	}

	public String getTreeType() {		return treeType;	}

	@Override
	protected void dumpChildren(int i)
	{
		children.values().forEach( child -> { child.dump(i); } );
	}

	@Override
	public String toString()
	{
		return "tree "+treeType;
	}

	
	@Override
	public void preprocess() throws PlcException 
	{
		if( treeType.equals("T_FN_DEF") ) 
		{
			BicAny statements = lookupChildByName("tFNDEF_STMTS");
			BicChain args = (BicChain) lookupChildByName("tFNDEF_ARGS");
			BicTree name = (BicTree) lookupChildByName("tFNDEF_NAME");
			BicAny retType = lookupChildByName("tFNDEF_RET_TYPE");

			unlinkChildByName("tFNDEF_NAME");
			unlinkChildByName("tFNDEF_RET_TYPE");
			unlinkChildByName("tFNDEF_ARGS");
			
			if( (name == null) || (args == null) || (statements == null) || (retType == null) )
				throw new PlcException("T_FN_DEF", "incomplete children");

			//IdentNode inode = (IdentNode)name.getNode();
			//String funcName = inode.getName();
			//if( !name.getTreeType().equals("T_IDENTIFIER") ) throw new PlcException("prprocess T_FN_DEF", "not T_IDENTIFIER");
			name.assertTypeIs("T_IDENTIFIER");
			BicStringConst namev = (BicStringConst) name.lookupChildByName("tID_STR");
			String funcName = namev.getValue();

			PhantomType pt = cToPhantomType((BicTree) retType);
			
			System.out.println("def fun \""+funcName+"\", type is "+pt);
			
			// -------------------------------------------------------------
			// Args
			// -------------------------------------------------------------

			args.children.forEach(  arg -> {
				ArgDefinitionNode argn;
				try {
					argn = loadArg((BicTree) arg);
					System.out.println(argn);
				} catch (PlcException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
					System.exit(22);
				}
				} ); 
			
			
			
			return;
		}		
	}	

	private ArgDefinitionNode loadArg(BicTree arg) throws PlcException {
		arg.assertTypeIs("T_DECL");
	
		BicTree typeTree = (BicTree) arg.lookupChildByName("tDECL_TYPE");
		BicTree nameTree = (BicTree) arg.lookupChildByName("tDECL_DECLS");
		
		int pointerDepth = 0;
		
		while(nameTree.typeIs("T_POINTER"))
		{
			pointerDepth++;
			nameTree = (BicTree) nameTree.lookupChildByName("tPTR_EXP");
		}

		if(pointerDepth > 0)
			System.err.println("Pointers not processed: "+pointerDepth);
		
		nameTree.assertTypeIs("T_IDENTIFIER");
		BicStringConst namev = (BicStringConst) nameTree.lookupChildByName("tID_STR");
		
		String arg_name = namev.getValue();
		PhantomType arg_type = cToPhantomType(typeTree);
		
		ArgDefinitionNode an = new ArgDefinitionNode( arg_name, arg_type );
		
		return an;
	}

	private boolean typeIs(String type) {
		return getTreeType().equals(type);
	}

	private void assertTypeIs(String type) throws PlcException {
		if( !getTreeType().equals(type) ) throw new PlcException("Tree Type", "not "+type);		
	}

	/**
	 * 
	 * Convert Bic AST node to PLC Node.
	 * 
	 * This is where most of the translation is done.
	 * 
	 */
	
	@Override
	protected void expressionToNode() throws PlcException 
	{
		if( treeType.equals("T_INTEGER") ) 
		{ 
			node = getOnlyChildAsTree().getNode(); 
			assert(node != null); 
			deleteChildren(); 
			return; 
		}
				
		if( treeType.equals("T_IDENTIFIER") ) 
		{ 
			BicStringConst child = (BicStringConst) getOnlyChildAsTree();
			node = new IdentNode(child.getValue(), getParseState() );
			deleteChildren(); 
			return;
		}		
						
		if( treeType.equals("T_ASSIGN") ) 
		{ 
			BicTree expr = (BicTree) lookupChildByName("tASSIGN_RHS");
			BicTree lval = (BicTree) lookupChildByName("tASSIGN_LHS");
			
			Node exprn = expr.getNode();
			if( exprn == null ) throw new PlcException("assign", "no expr");

			if( lval.getNode() != null )
			{
				node = new OpAssignNode(lval.getNode(), exprn);
				return;
			}
			
			/*
			if(lval.typeIs("T_IDENTIFIER"))
			{
				node = Assi
			}*/
			
			//return; 
			throw new PlcException("assign", "no lval");
		}
		
		if( treeType.equals("T_DECL") ) 
		{ 
			BicTree type = (BicTree) lookupChildByName("tDECL_TYPE");
			BicChain expr = (BicChain) lookupChildByName("tDECL_DECLS");
		
			PhantomType pt = cToPhantomType(type);
			
			StatementsNode snode = new StatementsNode();
			node = snode;
			
			expr.children.forEach( child -> {
				BicTree tchild = (BicTree) child;
				try {
					tchild.declareExprVar(pt);
				} catch (PlcException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				snode.addNode(child.getNode());
				} );
			return;
		}
		
		Class<? extends Node> binOpClass = binaryOps.get(treeType);
		if(binOpClass != null)
		{
			try {
				Constructor<? extends Node> ctor = binOpClass.getConstructor(Node.class, Node.class);
				node = ctor.newInstance(getLeftChildNode(), getRightChildNode());				
			} catch (NoSuchMethodException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (SecurityException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (InstantiationException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IllegalAccessException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IllegalArgumentException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (InvocationTargetException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			return;			
		}

		Class<? extends Node> unOpClass = unaryOps.get(treeType);
		if(unOpClass != null)
		{
			try {
				Constructor<? extends Node> ctor = unOpClass.getConstructor(Node.class);
				Node cnode = getOnlyChildAsTree().getNode();
				node = ctor.newInstance(cnode);				
			} catch (NoSuchMethodException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (SecurityException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (InstantiationException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IllegalAccessException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IllegalArgumentException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (InvocationTargetException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			return;			
		}
		
		
		System.err.println("expressionToNode(): UNKNOWN tree \""+treeType+"\"");
	}

	/**
	 * Called in func operators chain / T_DECL / tDECL_DECLS chain / element.
	 * 
	 * Syntactically it is C "TYPE var = expr;", we are called for "var = expr" part. 
	 * 
	 * @param pt
	 * @throws PlcException 
	 */
	private void declareExprVar(PhantomType pt) throws PlcException 
	{
		assertTypeIs("T_ASSIGN");
		
		BicTree id = (BicTree) lookupChildByName("tASSIGN_LHS");
		id.assertTypeIs("T_IDENTIFIER");
		
		BicStringConst is = (BicStringConst) id.lookupChildByName("tID_STR");
		
		String name = is.getValue();	}

	// -------------------------------------------------------------------
	// Children
	// -------------------------------------------------------------------
	
	private Node getLeftChildNode() {
		
		BicAny c = getLeftChild();
		//assert( c.node != null );
		return c.node;
	}

	private Node getRightChildNode() {
		
		BicAny c = getRightChild();
		//assert( c.node != null );
		return c.node;
	}

	private BicAny getLeftChild() { return lookupChildBySuffix("LHS"); }

	private BicAny getRightChild() { return lookupChildBySuffix("RHS"); }

	private BicAny lookupChildBySuffix(String string) 
	{		
		BicAny[] ret = {null};
		
		children.forEach( (key, val) -> {
			if( key.endsWith(string)) ret[0] = val;
		});
		
		return ret[0];
	}

	private BicAny lookupChildByName(String name) throws PlcException 
	{		
		BicAny[] ret = {null};
		
		children.forEach( (key, val) -> {
			if( key.equals(name)) ret[0] = val;
		});
		
		if(ret[0] == null) throw new PlcException("lookupChildByName", "no "+name);
		
		return ret[0];
	}

	private void unlinkChildByName(String name) {
		children.remove(name);		
	}

	
	
	private void deleteChildren() {
		children.clear();		
	}

	private BicAny getOnlyChildAsTree() throws PlcException 
	{
		if( children.size() != 1) throw new PlcException("getOnlyChildAsTree()", "not only");	
		return children.values().iterator().next();
	}	

	
	// -------------------------------------------------------------------
	// Binary ops
	// -------------------------------------------------------------------
	
	static private final HashMap<String,Class<? extends Node>> binaryOps = new HashMap<>();
	static {
		binaryOps.put("T_ADD", OpPlusNode.class);
	}

	// -------------------------------------------------------------------
	// Unary ops
	// -------------------------------------------------------------------
	
	static private final HashMap<String,Class<? extends Node>> unaryOps = new HashMap<>();
	static {
		unaryOps.put("T_RETURN", ReturnNode.class);
	}
	
	// -------------------------------------------------------------------
	// Types
	// -------------------------------------------------------------------

	private PhantomType cToPhantomType(BicTree retType) {
		
		String tt = retType.getTreeType();
		
		//System.out.println("cToPhantomType: got "+tt);
		
		if( tt.equals("D_T_INT") ) 			return PhantomType.getInt();
		if( tt.equals("D_T_LONG") )			return PhantomType.getLong();
		if( tt.equals("D_T_FLOAT") ) 		return PhantomType.getFloat();
		if( tt.equals("D_T_DOUBLE") )		return PhantomType.getDouble();
		
		if( tt.equals("D_T_CHAR") ) 		return PhantomType.getInt(); // TODO char?!
		
		System.err.println("cToPhantomType: unknown "+tt);
		
		//return null;
		return PhantomType.getUnknown();
	}

	
	
}


