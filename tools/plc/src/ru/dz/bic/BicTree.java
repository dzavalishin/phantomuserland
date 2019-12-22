package ru.dz.bic;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.function.Consumer;

import ru.dz.plc.compiler.binode.OpPlusNode;
import ru.dz.plc.compiler.node.Node;
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
	protected void expressionToNode() throws PlcException 
	{
		if( treeType.equals("T_INTEGER") ) 
		{ 
			node = getOnlyChildAsTree().getNode(); 
			assert(node != null); 
			deleteChildren(); 
			return; 
		}
		
		Class binOpClass = binaryOps.get(treeType);
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
		
		System.err.println("expressionToNode(): UNKNOWN tree \""+treeType+"\"");
	}

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

	private BicAny getLeftChild() { return lookupChild("LHS"); }

	private BicAny getRightChild() { return lookupChild("RHS"); }

	private BicAny lookupChild(String string) 
	{		
		BicAny[] ret = {null};
		
		children.forEach( (key, val) -> {
			if( key.endsWith(string)) ret[0] = val;
		});
		
		return ret[0];
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
	
	static private final HashMap<String,Class> binaryOps = new HashMap<>();
	static {
		binaryOps.put("T_ADD", OpPlusNode.class);
	}
	
}


