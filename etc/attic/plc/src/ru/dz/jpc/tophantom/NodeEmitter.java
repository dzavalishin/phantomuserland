package ru.dz.jpc.tophantom;

import java.util.LinkedList;
import java.util.List;
import java.util.logging.Logger;

import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.node.EmptyNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;

public class NodeEmitter {
	Logger log = Logger.getLogger(NodeEmitter.class.getName());

	private List<Node> out = new LinkedList<Node>();	
	private List<Node> stack = new LinkedList<Node>();

	public void push( Node n )	
	{
		log.finer("push "+n);
		stack.add(0, n);	
	}

	public Node pop()	
	{				
		log.finer("pop "+stack.get(0));
		return stack.remove(0);	
	}



	//public List<Node> getList() { return out; }

	public Node getCodeTree()
	{
		if( !stack.isEmpty() )
			emitLinear(new EmptyNode()); // kick off any stack contents

		Node code = out.isEmpty() ? new EmptyNode() : out.remove(out.size()-1); // Get last one

		while(!out.isEmpty())
		{
			//code = new SequenceNode( code, out.remove(out.size()-1) );
			code = new SequenceNode( out.remove(out.size()-1), code );
		}
		return code;
	}

	/**
	 * If stack is not empty, emit all the nodes from it. Emit given node.
	 * It is supposed that stack keeps head nodes of all expression calculations
	 * between flow control operators. 
	 * @param n node to emit.
	 */
	public void emitLinear(Node n) 
	{
		if(!stack.isEmpty()) 
		{
			log.finer("emitLinear stack flush");
			flushStack();
			log.finer("emitLinear stack flush end");
		}
		if(stack.isEmpty())
		{
			log.finer("emitLinear "+n);
			out.add(n);
			return;
		}
	} 

	private void flushStack()
	{
		Node sn = pop();
		if(!stack.isEmpty()) flushStack();
		log.finer("emitLinear flush "+sn);
		out.add(sn);		
	}

}
