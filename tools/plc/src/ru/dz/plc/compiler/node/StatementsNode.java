package ru.dz.plc.compiler.node;

import java.io.IOException;
import java.io.PrintStream;
import java.util.LinkedList;
import java.util.List;
import java.util.function.Consumer;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * <p>Sequence of statements. Just outputs them sequentally.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2013 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */


public class StatementsNode extends Node {
	private List<Node> nodes = new LinkedList<Node>();
	
	public void addNode(Node n) { 
		//print_warning("add "+n);
		//print_warning("sz "+nodes.size());
		nodes.add(n); 
		}
	
	public StatementsNode() {		super(null);	}
	public boolean args_on_int_stack() { return false; }
	public String toString()  {    return "...";  }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
	}

	public PhantomType find_out_my_type()  {    return PhantomType.getVoid();  }

	public void propagateVoidParents()
	{
		for( Node n : nodes )
		{
			n.setParentIsVoid();
			n.propagateVoidParents();
		}
	}
	
	
	
	@Override
	public void generate_code(Codegen c, CodeGeneratorState s)
			throws IOException, PlcException {

		//print_warning("sz "+nodes.size());
		for( Node n : nodes )
		{
			//print_warning("codegen "+n);
			n.generate_code(c, s);
		}

	}
	
	@Override
	public void generateLlvmCode(LlvmCodegen llc) throws PlcException {
		for( Node n : nodes )
			n.generateLlvmCode(llc);
	}
	
	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException {
		for( Node n : nodes )
		{
			n.generate_C_code(cgen, s);
			cgen.putln(";");
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

	
	public void forEach(Consumer<? super Node> action) {
		nodes.forEach(action);		
	}

	public List<Node> getNodeList() {
		return nodes;		
	}
	
	
}
