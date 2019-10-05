package ru.dz.jpc.python;

import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.binode.CallArgNode;
import ru.dz.plc.compiler.binode.NewNode;
import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.binode.OpPlusNode;
import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.binode.ValEqNode;
import ru.dz.plc.compiler.node.EmptyNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.JumpNode;
import ru.dz.plc.compiler.node.JumpTargetNode;
import ru.dz.plc.compiler.node.JzNode;
import ru.dz.plc.compiler.node.MethodNode;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.compiler.node.OpNotNode;
import ru.dz.plc.compiler.node.ReturnNode;
import ru.dz.plc.compiler.trinode.OpMethodCallNode;
import ru.dz.plc.util.PlcException;

public class PythonFrontendXML {
	private static final boolean really = false;
	DocumentBuilderFactory docBuilderFactory;
	DocumentBuilder docBuilder;
	private Document doc;
	
	/** Current file name */
	private String fileName = "";
	/** Current line number */
	private int currentLineNo = -1;
	/** Current line text */
	private String currentLineText = "";
	private String funcName;
	
	public PythonFrontendXML() throws ParserConfigurationException {	
        docBuilderFactory = DocumentBuilderFactory.newInstance();
        docBuilder = docBuilderFactory.newDocumentBuilder();				
	}
	
	
	void load(File fn) throws ParserConfigurationException, SAXException, IOException
	{
        doc = docBuilder.parse(fn);		
	}
	
	public void print()
	{
		//System.out.println(doc.toString());
		
		Node documentElement = doc.getDocumentElement();
		
		printNode(documentElement,0);
	}


	private void printNode(Node n, int level) {
	
		switch( n.getNodeType() )
		{
		case Node.CDATA_SECTION_NODE: 
		case Node.COMMENT_NODE:
		//case Node.ENTITY_NODE:
		//case Node.DOCUMENT_FRAGMENT_NODE:
			return;
		}
		
		if(n.getNodeName().equals("#text"))
			return;
		
		for( int l = 0; l <level; l++) System.out.print(' ');
		//System.out.println(n.toString());
		System.out.println(n.getNodeName().toLowerCase());
		
		NodeList childNodes = n.getChildNodes();
		
		
		
		for( int i = 0; i < childNodes.getLength(); i++  )
		{
			Node cn = childNodes.item(i);
			
			printNode(cn, level+2);
		}
	}


	public void convert() throws ConnvertException, PlcException {
		Node de = doc.getDocumentElement();
		
		if(!de.getNodeName().equalsIgnoreCase("pythonfrontend"))
			throw new ConnvertException("top element is "+de.getNodeName()+", not pythonfrontend");
		
		NodeList childNodes = de.getChildNodes();
		
		for( int i = 0; i < childNodes.getLength(); i++  )
		{
			Node cn = childNodes.item(i);

			String nname = cn.getNodeName().toLowerCase();
			
			if(nname.equals("#text"))
				continue;
			else if(nname.equals("label"))
			{
				PythonLabel lbl = new PythonLabel(cn);
				//System.out.println("Top level label: "+lbl);
			}
			else if(nname.equals("pythoncode"))
			{
				loadCode(cn);
			}
			else
			{
				System.out.println("Unknown top node: "+nname);
			}
		}
		
	}


	private Map<Integer,RegisterNodeWrapper> registers = new HashMap<Integer, RegisterNodeWrapper>();
	
	private void setRegister(int reg, ru.dz.plc.compiler.node.Node n )
	{
		registers.put(reg, new RegisterNodeWrapper(n) );
	}

	private void setRegister(int reg, RegisterNodeWrapper rn )
	{
		registers.put(reg, rn );
	}

	private void setRegister(Node cn, String name, ru.dz.plc.compiler.node.Node n ) {
		registers.put(getInt(cn, name), new RegisterNodeWrapper(n) );
	}
	
	private RegisterNodeWrapper getRegister( int reg )
	{
		return registers.get(reg);
	}

	private ru.dz.plc.compiler.node.Node useRegister( int reg )
	{
		return registers.get(reg).use();
	}

	private ru.dz.plc.compiler.node.Node useRegister( Node cn, String name )
	{
		return registers.get(getInt(cn, name)).use();
	}

	
	private void loadCode(Node n) throws ConnvertException, PlcException {
		// We're in the top level code

		List<ru.dz.plc.compiler.node.Node> out = new LinkedList<ru.dz.plc.compiler.node.Node>();
		
		// These two are saved in DefFunc and used in PythonCode
		int funcOutReg = -1;
		int funcCodeLen = -1;
		
		// This one is defined in if node and used in jump node
		int ifReg = -1;
		
		NodeList childNodes = n.getChildNodes();
		for( int i = 0; i < childNodes.getLength(); i++  )
		{
			Node cn = childNodes.item(i);

			String nname = cn.getNodeName().toLowerCase();

			if(nname.equals("#text"))
				continue;
			if(nname.equals("#comment"))
				continue;
			/*else if(nname.equals("label"))
			{
				PythonLabel lbl = new PythonLabel(cn);
				System.out.println("Label: "+lbl);
			}*/
			else if(nname.equals("pos"))
			{
				String line = cn.getAttributes().getNamedItem("line").getNodeValue();
				String text = cn.getAttributes().getNamedItem("text").getNodeValue();

				//System.out.println("Pos: line="+line+" text=\""+text+"\"");
				
				currentLineNo = Integer.parseInt(line);
				currentLineText = text;
			}			
			else if(nname.equals("file"))
			{
				try {
				String name = cn.getAttributes().getNamedItem("name").getNodeValue();
				fileName = name;
				//System.out.println("File \""+name+"\"");
				} catch( Throwable e )
				{ /* Ignore */ }
			}			
			else if(nname.equals("function"))
			{
				String name = cn.getAttributes().getNamedItem("name").getNodeValue();
				funcName = name;
				//System.out.println("Func \""+name+"\"");
			}			
			else if(nname.equals("regs"))
			{
				String num = cn.getAttributes().getNamedItem("num").getNodeValue();
				//System.out.println("Func \""+name+"\"");
			}			
			else if(nname.equals("string"))
			{
				setRegister(getInt(cn,"reg"), new ru.dz.plc.compiler.node.StringConstNode(getString(cn,"content")));
			}			
			else if(nname.equals("number"))
			{
				// TODO Its FLOAT!
				setRegister(getInt(cn,"reg"), new ru.dz.plc.compiler.node.IntConstNode((int)getDouble(cn,"val")));
			}			
			else if(nname.equals("eof"))
			{
				// empty
			}
			else if(nname.equals("dict"))
			{
				int outReg = 0;
				int inStartReg = 0;
				int inRegsNum = 0;
				
				if(haveParam(cn, "inNum"))
				{
					outReg = getInt(cn,"out");
					inStartReg = getInt(cn,"inStart");
					inRegsNum = getInt(cn,"inNum");
				}
				else
					outReg = getInt(cn,"reg");
				
				setRegister(outReg, new NewNode(new PhantomType(ClassMap.get_map().get(".internal.container.array",false,null)), null, null) );
				// TODO use some map style container for dictionary
				// TODO here we must fill a new dictionary - compose?
				if(inRegsNum != 0)
					throw new ConnvertException("No dictionary compose code yet");
			}
			else if(nname.equals("gget"))
			{
				// TODO this is wrong!
				String varName = getString(cn, "gVarName");
				setRegister(getInt(cn,"to"), new IdentNode(varName) );
				if( really ) throw new ConnvertException(nname+" is not implemented");
			}
			else if(nname.equals("gset"))
			{
				// TODO this is wrong!
				String varName = getString(cn, "gVarName");
				out.add( new OpAssignNode(new IdentNode(varName), useRegister(getInt(cn,"fromreg"))) );
				if( really ) throw new ConnvertException(nname+" is not implemented");
			}
			else if(nname.equals("get"))
			{
				int toReg = getInt(cn,"toreg");
				int objectReg = getInt(cn,"class");
				int nameregReg = getInt(cn,"fieldName");

				/*
				int toReg = getInt(cn,"out");
				int objectReg = getInt(cn,"left");
				int nameregReg = getInt(cn,"right");
				*/
				
				// TODO implement
				setRegister(toReg, new EmptyNode() );
				if( really ) throw new ConnvertException(nname+" is not implemented");
			}
			else if(nname.equals("set"))
			{
				int fromReg = getInt(cn,"fromreg");
				int objectReg = getInt(cn,"class");
				String nameregReg = getString(cn,"fieldName");
				// TODO implement
				if( really ) throw new ConnvertException(nname+" is not implemented");
			}
			else if(nname.equals("move"))
			{
				setRegister(getInt(cn,"to"), getRegister(getInt(cn,"from")));
			}
			else if(nname.equals("return"))
			{
				out.add( new ReturnNode( useRegister(getInt(cn, "reg")) ) );
			}
			else if(nname.equals("add"))
			{				
				setRegister(cn, "out", new OpPlusNode(useRegister(cn,"left"),useRegister(cn,"right")) );
			}
			else if(nname.equals("eq"))
			{				
				setRegister(cn, "out", new ValEqNode(useRegister(cn,"left"),useRegister(cn,"right")) );
			}
			else if(nname.equals("if"))
			{
				if(ifReg >= 0)
					System.out.println("PythonFrontendXML.loadCode() pref if unused!!");
				ifReg = getInt(cn, "reg");
			}
			else if(nname.equals("label"))
			{				
				out.add(  new JumpTargetNode(getString(cn,"name")) );
			}
			else if(nname.equals("jump"))
			{
				String label = getString(cn, "label");
				if(ifReg >= 0)
				{
					out.add(new SequenceNode(new OpNotNode( getRegister(ifReg).use() ), new JzNode(label)) );
					ifReg = -1;
				}
				else					
					out.add(  new JumpNode(label) );				
			}
			else if(nname.equals("none"))
			{				
				setRegister(getInt(cn,"reg"),  new NullNode() );
			}
			else if(nname.equals("params"))
			{		
				int reg = getInt(cn,"inStart");
				int count = getInt(cn,"inNum");
				
				ru.dz.plc.compiler.node.Node args = new EmptyNode();
				
				while(count-- > 0)
				{
					args = new CallArgNode(getRegister(reg++).use(), args);
				}
				
				
				setRegister(getInt(cn,"out"),  args );
			}
			else if(nname.equals("call"))
			{				
				ru.dz.plc.compiler.node.Node func = new MethodNode(0); //getRegister(getInt(cn,"func")).use();
				ru.dz.plc.compiler.node.Node args = getRegister(getInt(cn,"params")).use();
				ru.dz.plc.compiler.node.Node object = new MethodNode(0); // TODO wrong
				setRegister(getInt(cn,"ret"),  new OpMethodCallNode(object, func, args) );
			}
			else if(nname.equals("deffunc"))
			{
				funcOutReg = getInt(cn, "outreg");
				funcCodeLen = getInt(cn, "len");
			}
			else if(nname.equals("pythoncode"))
			{
				// TODO and what now?
				processFunction(cn,funcOutReg,funcCodeLen);
			}
			else
			{
				System.out.println("Unknown node: "+nname);
			}
			
		}	
		
		PrintStream ps = System.out;
		//PhantomClass pc = new PhantomClass("?PythonClass"); // TODO wrong
		PhantomClass pc = new PhantomClass(".internal.object"); // TODO WRONG!
		ParseState pstate = new ParseState(pc );

		pstate.set_method(new Method("mname", new PhantomType(new PhantomClass(".internal.object"))));
		
		for( ru.dz.plc.compiler.node.Node in : out )
		{
			in.preprocess(pstate);
			in.print(ps);
			ps.format("\n-\n");
		}
	}




	private void processFunction(Node cn, int funcOutReg, int funcCodeLen) throws ConnvertException, PlcException {		
		loadCode(cn);
	}


	private String getString(Node cn, String name) {
		return cn.getAttributes().getNamedItem(name).getNodeValue();		
	}

	private boolean haveParam(Node cn, String name) {
		return cn.getAttributes().getNamedItem(name) != null;		
	}

	private int getInt(Node cn, String name) {
		return Integer.parseInt(cn.getAttributes().getNamedItem(name).getNodeValue());		
	}
	
	private double getDouble(Node cn, String name) {
		return Double.parseDouble(cn.getAttributes().getNamedItem(name).getNodeValue());		
	}
	
}


