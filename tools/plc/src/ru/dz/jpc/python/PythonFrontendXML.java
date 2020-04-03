package ru.dz.jpc.python;

import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.binode.NewNode;
import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.binode.OpPlusNode;
import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.binode.ValEqNode;
import ru.dz.plc.compiler.node.BoolNotNode;
import ru.dz.plc.compiler.node.EmptyNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.JumpNode;
import ru.dz.plc.compiler.node.JumpTargetNode;
import ru.dz.plc.compiler.node.JzNode;
import ru.dz.plc.compiler.node.MethodNode;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.compiler.node.ReturnNode;
import ru.dz.plc.compiler.node.StatementsNode;
import ru.dz.plc.util.PlcException;

public class PythonFrontendXML {
	static final Logger log = Logger.getLogger("ru.dz.jpc.python");

	private int errorCount = 0;
	private PhantomClass pc = null;

	private ParseState				ps = new ParseState();

	
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
	//private String funcName;
	
	private Map<Integer,RegisterNodeWrapper> registers = new HashMap<Integer, RegisterNodeWrapper>();

	private String className = null;

	
	void incErrors(String msg) 
	{
		errorCount++;
		log.severe(msg);
	}
	
	

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
		System.out.print(n.getNodeName().toLowerCase());
		//System.out.println(n.getAttributes().getLength());
		NamedNodeMap atrs = n.getAttributes();
		if( atrs != null )
		{
			int na = atrs.getLength();
			for( int ai = 0; ai < na; ai ++ )
				System.out.print( " "+atrs.item(ai) );
			
		}
		System.out.println();
		
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
				log.log(Level.INFO, "Top level label: "+lbl );
			}
			else if(nname.equals("pythoncode"))
			{
				loadCode(cn);
			}
			else
			{
				System.out.println("Unknown node: "+nname);
			}
		}
		
	}



	
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

	private PythonFunctionParseState pps = new PythonFunctionParseState();
	
	private void loadCode(Node n) throws ConnvertException, PlcException {
		// We're in the top level code

		List<ru.dz.plc.compiler.node.Node> out = new LinkedList<ru.dz.plc.compiler.node.Node>();
		
		// These two are saved in DefFunc and used in PythonCode
		//int funcOutReg = -1;
		//int funcCodeLen = -1;
		//String funcName = null;
		
		
		NodeList childNodes = n.getChildNodes();
		for( int i = 0; i < childNodes.getLength(); i++  )
		{
			Node cn = childNodes.item(i);

			processOne(out, cn);
			
		}
		
		// Finished processing <pythoncode>...</pythoncode>, now create method
		if(null==pps.getFuncName())
		{
			log.log(Level.SEVERE,"No function name at the end of code");
			return;
		}

		if(null==pc)
		{
			log.log(Level.SEVERE,"No class at the end of code");
			return;
		}
		
		// TODO type is hardcoded - ERROR FIXME
		//Method m = new Method(pps.getFuncName(), PhantomType.t_string, false); // false = not constructor
		Method m = new Method(pps.getFuncName(), PhantomType.getUnknown(), false); // false = not constructor
		pc.addMethod(m);
		
		//m.code = useRegister(funcOutReg);

		/*
		ru.dz.plc.compiler.node.Node last = new NullNode();
		
		for( ru.dz.plc.compiler.node.Node ln : out )
		{
			last = new SequenceNode(last, ln);			
		}
		*/
		
		StatementsNode last = new StatementsNode(); 
		for( ru.dz.plc.compiler.node.Node ln : out )
			last.addNode(ln);
		
		//last.print(new PrintStream(System.out));
		
		m.code = last;
		
		
	}


	/**
	 * Process one PyFront statement
	 * 
	 * @param out - where to add generated code (plc nodes)
	 * @param cn - XML tree node to convert
	 * @throws PlcException
	 * @throws ConnvertException
	 */

	public void processOne(List<ru.dz.plc.compiler.node.Node> out, Node cn) throws PlcException, ConnvertException {
		String nname = cn.getNodeName().toLowerCase();

		if(nname.equals("#text"))
			return;
		if(nname.equals("#comment"))
			return;
		else if("class".equalsIgnoreCase(nname))
		{
			String name = cn.getAttributes().getNamedItem("name").getNodeValue();
			className = name;
			log.log(Level.INFO, "Class \""+name+"\"" );
			ClassMap cm = ClassMap.get_map();
			
			pc = new PhantomClass(className);
			cm.add(pc);
			ps.set_class(pc);
		}			
		else if(nname.equals("pos"))
		{
			String line = cn.getAttributes().getNamedItem("line").getNodeValue();
			String text = cn.getAttributes().getNamedItem("text").getNodeValue();

			//System.out.println("Pos: line="+line+" text=\""+text+"\"");
			
			currentLineNo = Integer.parseInt(line);
			currentLineText = text;
			log.log(Level.INFO,"pos "+currentLineNo+" ("+currentLineText+")");
			
			// TODO set node contexts from here
			
			// TODO hack - we reparse python src, generate class name XML node instead!
			/*
			text = text.trim();
			if( text.toLowerCase().startsWith("class") )
			{
				String cname = text.substring(5).trim();
				if( cname.endsWith(":") )
				{
					cname = cname.substring(0, cname.length()-1);
					log.info("class def: \""+cname+"\" at "+currentLineNo);
					
					ClassMap cm = ClassMap.get_map();
					
					// Sometimes class def string duplicates!
					if(cm.get(cname, true, null) == null)
					{
						pc = new PhantomClass(cname);
						cm.add(pc);
						ps.set_class(pc);
					}
				}
				else
				{
					log.severe("Unparsable class def: "+currentLineText+" at "+currentLineNo);
				}
			}
			*/
			
		}			
		else if("file".equals(nname))
		{
			try {
			String name = cn.getAttributes().getNamedItem("name").getNodeValue();
			fileName = name;
			//System.out.println("File \""+name+"\"");
			log.log(Level.INFO, "File \""+fileName+"\"" );
			} catch( Throwable e )
			{ /* Ignore */ }
		}			
		else if("call".equals(nname))
		{
			int nFuncReg = getInt(cn, "func"); // this register contains func name
			String ident = "unknown";
			setRegister(getInt(cn,"ret"), new MethodNode(ident, ps) );
		}
		else if("function".equals(nname))
		{
			String name = cn.getAttributes().getNamedItem("name").getNodeValue();
			pps.setFuncName( name );
			log.log(Level.INFO,"Func \""+pps.getFuncName()+"\"");
		}			
		else if("regs".equals(nname))
		{
			String num = cn.getAttributes().getNamedItem("num").getNodeValue();
			log.log(Level.INFO,"regs "+num);
			int nregs;

			try { nregs = Integer.parseInt(num); }
			catch( NumberFormatException e )
			{
				nregs = 0;
				incErrors("regs arg not parsable: "+num);
			}
			log.log(Level.INFO,"regs "+nregs);
		}			
		else if("string".equals(nname))
		{
			//setRegister(getInt(cn,"reg"), new ru.dz.plc.compiler.node.StringConstNode(getString(cn,"content")));
			setRegister(getInt(cn,"reg"), new ru.dz.plc.compiler.node.StringConstPoolNode(getString(cn,"content"),pc));
		}			
		else if("number".equals(nname))
		{
			//setRegister(getInt(cn,"reg"), new ru.dz.plc.compiler.node.IntConstNode((int)getDouble(cn,"val")));
			setRegister(getInt(cn,"reg"), new ru.dz.plc.compiler.node.IntConst64Node(getDouble(cn,"val")));
		}			
		else if("eof".equals(nname))
		{
			// empty
		}
		else if("dict".equals(nname))
		{
			if( cn.getAttributes().getNamedItem("reg") != null )
			{
				int outReg = getInt(cn,"reg");
				setRegister(outReg, new NewNode(new PhantomType(ClassMap.get_map().get(".internal.container.array",false,null)), null, null) );
			}
			else
			{
			int outReg = getInt(cn,"out");
			int inStartReg = getInt(cn,"inStart");
			int inRegsNum = getInt(cn,"inNum");

			log.log(Level.INFO,"dict inStart "+inStartReg);
			
			setRegister(outReg, new NewNode(new PhantomType(ClassMap.get_map().get(".internal.container.array",false,null)), null, null) );
			// TODO use some map style container for dictionary
			// TODO here we must fill a new dictionary - compose?
			if(inRegsNum != 0)
				throw new ConnvertException("No dictionary compose code yet");
			}
		}
		else if("gget".equals(nname))
		{
			// TODO this is wrong!
			String varName = getString(cn, "gVarName");
			setRegister(getInt(cn,"to"), new IdentNode(varName,ps) );
			if( really ) throw new ConnvertException(nname+" is not implemented");
		}
		else if("gset".equals(nname))
		{
			// TODO this is wrong!
			String varName = getString(cn, "gVarName");
			out.add( new OpAssignNode(new IdentNode(varName,ps), useRegister(getInt(cn,"fromreg"))) );
			if( really ) throw new ConnvertException(nname+" is not implemented");
		}
		else if("get".equals(nname))
		{
			int toReg = -1;
			int objectReg = -1;
			int nameregReg = -1;
			
			if( cn.getAttributes().getNamedItem("toreg") != null )
			{
				toReg = getInt(cn,"toreg");
				objectReg = getInt(cn,"class");
				nameregReg = getInt(cn,"fieldName");
			}
			else
			{
				toReg = getInt(cn,"out");				
				objectReg = getInt(cn,"left");
				nameregReg = getInt(cn,"right");
			}
			
			log.log(Level.INFO,"get to "+toReg+" class "+objectReg+" field "+nameregReg );
			
			// TODO implement
			setRegister(toReg, new EmptyNode() );
			if( really ) throw new ConnvertException(nname+" is not implemented");
		}
		else if("set".equals(nname))
		{
			int fromReg = getInt(cn,"fromreg");
			int objectReg = getInt(cn,"class");
			//int nameregReg = getInt(cn,"fieldName");
			String fieldName = getString(cn, "fieldName");
			
			log.log(Level.INFO,"set from "+fromReg+" class "+objectReg+" field "+fieldName );
			
			// TODO implement
			if( really ) throw new ConnvertException(nname+" is not implemented");
		}
		else if("move".equals(nname))
		{
			setRegister(getInt(cn,"to"), getRegister(getInt(cn,"from")));
		}
		else if("return".equals(nname))
		{
			out.add( new ReturnNode( useRegister(getInt(cn, "reg")) ) );
		}
		else if("add".equals(nname))
		{				
			setRegister(cn, "out", new OpPlusNode(useRegister(cn,"left"),useRegister(cn,"right")) );
		}
		else if("eq".equals(nname))
		{				
			setRegister(cn, "out", new ValEqNode(useRegister(cn,"left"),useRegister(cn,"right")) );
		}
		else if(nname.equals("ifjmp"))
		{
			ru.dz.plc.compiler.node.Node r = useRegister(cn, "reg");
			String label = getString(cn, "jmp");
			
			JzNode jn = new JzNode( new BoolNotNode( r ), label );
			out.add( jn );
		}
		else if("label".equals(nname))
		{				
			out.add(  new JumpTargetNode(getString(cn,"name")) );
		}
		else if("jump".equals(nname))
		{
			out.add(  new JumpNode(getString(cn, "label")) );				
		}
		else if("none".equals(nname))
		{				
			setRegister(getInt(cn,"reg"),  new NullNode() );
		}
		else if("deffunc".equals(nname))
		{
			pps.setFuncOutReg( getInt(cn, "outreg") );
			pps.setFuncCodeLen( getInt(cn, "len") );				
		}
		else if("pythoncode".equals(nname))
		{
			// TODO and what now?
			processFunction(cn,pps.getFuncOutReg(),pps.getFuncCodeLen());
		}
		else
		{
			System.out.println("Unknown node: "+nname);
		}
	}




	private void processFunction(Node cn, int funcOutReg, int funcCodeLen) throws ConnvertException, PlcException {		
		loadCode(cn);
	}


	private String getString(Node cn, String name) {
		return cn.getAttributes().getNamedItem(name).getNodeValue();		
	}


	private int getInt(Node cn, String name) {
		return Integer.parseInt(cn.getAttributes().getNamedItem(name).getNodeValue());		
	}
	
	private double getDouble(Node cn, String name) {
		return Double.parseDouble(cn.getAttributes().getNamedItem(name).getNodeValue());		
	}

	
	public int getErrorCount() { 		return errorCount;	}

}


