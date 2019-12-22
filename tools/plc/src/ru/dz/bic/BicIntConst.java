package ru.dz.bic;

import java.util.function.Consumer;

import ru.dz.plc.compiler.node.IntConstNode;

public class BicIntConst extends BicAny {

	private int value;

	public BicIntConst(int value) {
		this.value = value;
	}

	int getValue() { return value; }
	
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
		return "IntConst "+Integer.toString(value);
	}

	@Override
	protected void expressionToNode() {
		node = new IntConstNode(value);		
	}
	
}
