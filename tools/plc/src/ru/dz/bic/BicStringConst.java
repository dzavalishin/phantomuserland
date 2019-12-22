package ru.dz.bic;

import java.util.function.Consumer;

import ru.dz.plc.compiler.node.StringConstNode;
import ru.dz.plc.compiler.node.StringConstPoolNode;

public class BicStringConst extends BicAny {

	private String value;

	public BicStringConst(String value) {
		this.value = value;
	}

	public String getValue() { return value; }

	@Override
	void foreach(Consumer<? super BicAny> action) {
		action.accept(this);
	}
	
	@Override
	protected void dumpChildren(int i)
	{
	}

	@Override
	public String toString()
	{
		return "StringConst \""+ value + "\"";
	}

	
	@Override
	protected void expressionToNode() {
		//node = new StringConstPoolNode(); // TODO need class
		node = new StringConstNode( value );		
	}
}
