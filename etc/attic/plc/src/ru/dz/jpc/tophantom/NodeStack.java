package ru.dz.jpc.tophantom;

import java.util.LinkedList;
import java.util.List;

import ru.dz.plc.compiler.node.Node;

@Deprecated
public class NodeStack {
	private List<Node> l = new LinkedList<Node>();
	
	public void push( Node n )
	{
		l.add(0, n);
	}
	
	public Node pop()
	{
		return l.remove(0);
	}
	
}
