package ru.dz.plc.compiler.node;

import java.io.IOException;
import java.io.PrintStream;
import java.util.LinkedList;
import java.util.List;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.util.PlcException;

/**
 * <p>Sequence of statements. Just outputs them sequentally.</p>
 * <p>Copyright: Copyright (c) 2004-2013 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class StatementsNode extends Node {
	private List<Node> nodes = new LinkedList<Node>();
	
	public void addNode(Node n) { 
		//print_warning("add "+n);
		//print_warning("sz "+nodes.size());
		nodes.add(n); 
		}
	
	public StatementsNode() {		super(new EmptyNode());	}
	public boolean args_on_int_stack() { return false; }
	public String toString()  {    return "...";  }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
	}
	public void find_out_my_type()  {    type = new PhTypeVoid();  }

	
	
	@Override
	public void generate_code(Codegen c, CodeGeneratorState s)
			throws IOException, PlcException {

		//print_warning("sz "+nodes.size());
		for( Node n : nodes )
		{
			print_warning("codegen "+n);
			n.generate_code(c, s);
		}

	}
	
	@Override
	public void preprocess(ParseState s) throws PlcException {
		for( Node n : nodes )
			n.preprocess(s);
	}

	public void print( PrintStream ps, int level, int start_level ) throws PlcException
	{
		print_offset( ps, level, start_level );
		print_me(ps);
		for( Node n : nodes )
			n.print(ps, level+1, start_level+1 );
	}

	protected void print_me(PrintStream ps ) throws PlcException {
		ps.println(toString());
	}
	
	
}
