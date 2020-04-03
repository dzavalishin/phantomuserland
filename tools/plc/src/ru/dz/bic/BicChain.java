package ru.dz.bic;

import java.util.LinkedList;
import java.util.function.Consumer;

import ru.dz.plc.PlcMain;
import ru.dz.plc.util.PlcException;

/**
 * 
 * Corresponds to AST file <chain ...>
 * 
 * @author dz
 *
 */

public class BicChain extends BicAny {
	protected LinkedList<BicAny> children = new LinkedList<BicAny>(); 
	

	public void add(BicTree parseTree) {
		children.add(parseTree);		
	}

	
	@Override
	public void preprocess() 
	{
		// Just for each
		children.forEach( child -> { try {
			child.preprocess();
		} catch (PlcException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			System.exit(22);
		} } );
	}
	
	@Override
	void foreach(Consumer<? super BicAny> action) {
		//children.forEach(action);
		children.forEach( child -> { child.foreach(action); } );
		action.accept(this);
	}

	@Override
	protected void dumpChildren(int i)
	{
		children.forEach( child -> { child.dump(i); } );
	}
	
	@Override
	public String toString()
	{
		return "chain";
	}

	@Override
	protected void expressionToNode() {
		// TODO empty, must be processed by caller for result depends on context
	}	
}
