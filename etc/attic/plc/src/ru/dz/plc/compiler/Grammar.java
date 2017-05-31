package ru.dz.plc.compiler;

import java.io.*;

import ru.dz.plc.compiler.binode.BoolAndNode;
import ru.dz.plc.compiler.binode.BoolOrNode;
import ru.dz.plc.compiler.binode.CallArgNode;
import ru.dz.plc.compiler.binode.CatchNode;
import ru.dz.plc.compiler.binode.ForeachNode;
import ru.dz.plc.compiler.binode.NewNode;
import ru.dz.plc.compiler.binode.OpAndNode;
import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.binode.OpDivideNode;
import ru.dz.plc.compiler.binode.OpRemainderNode;
import ru.dz.plc.compiler.binode.OpMinusNode;
import ru.dz.plc.compiler.binode.OpMultiplyNode;
import ru.dz.plc.compiler.binode.OpOrNode;
import ru.dz.plc.compiler.binode.OpPlusNode;
import ru.dz.plc.compiler.binode.OpSubscriptNode;
import ru.dz.plc.compiler.binode.OpVassignNode;
import ru.dz.plc.compiler.binode.RefEqNode;
import ru.dz.plc.compiler.binode.RefNeqNode;
import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.binode.ValEqNode;
import ru.dz.plc.compiler.binode.ValGeNode;
import ru.dz.plc.compiler.binode.ValGtNode;
import ru.dz.plc.compiler.binode.ValLeNode;
import ru.dz.plc.compiler.binode.ValLtNode;
import ru.dz.plc.compiler.binode.ValNeqNode;
import ru.dz.plc.compiler.node.BinaryConstNode;
import ru.dz.plc.compiler.node.BoolNotNode;
import ru.dz.plc.compiler.node.BreakNode;
import ru.dz.plc.compiler.node.ClassDefinitionNode;
import ru.dz.plc.compiler.node.ContinueNode;
import ru.dz.plc.compiler.node.EmptyNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.IntConstNode;
import ru.dz.plc.compiler.node.MethodNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.compiler.node.OpNotNode;
import ru.dz.plc.compiler.node.ReturnNode;
import ru.dz.plc.compiler.node.StringConstNode;
import ru.dz.plc.compiler.node.SwitchCaseNode;
import ru.dz.plc.compiler.node.SwitchDefaultNode;
import ru.dz.plc.compiler.node.SwitchNode;
import ru.dz.plc.compiler.node.ThisNode;
import ru.dz.plc.compiler.node.ThrowNode;
import ru.dz.plc.compiler.node.VoidNode;
import ru.dz.plc.compiler.trinode.DoWhileNode;
import ru.dz.plc.compiler.trinode.IfNode;
import ru.dz.plc.compiler.trinode.OpMethodCallNode;
import ru.dz.plc.parser.*;
import ru.dz.plc.util.*;

/**
 * <p>Title: Phantom language parser</p>
 * <p>Description: Old and dirty recursive implementation.<br> Has to be 
 *    rewritten with some parser generator.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class Grammar
extends GrammarHelper {

	private ClassMap				classes = new ClassMap();

	private String					package_name = null;

	private ParseState				ps = new ParseState();

	private final boolean    		parser_debug = false;
	//private final boolean    parser_debug = true;



	private final int id_ident, id_string_const, id_int_const, id_this, id_null;

	private final int id_point, id_comma, id_at, id_src_req, id_dst_req;
	private final int id_colon, id_semicolon, id_assign, id_vassign;
	private final int id_eq, id_neq, id_gt, id_lt, id_ge, id_le;

	private final int id_ref_eq, id_ref_neq;
	private final int id_plus, id_minus, id_slash, id_aster, id_percent;
	private final int id_bar, id_caret, id_amper, id_tilde;
	private final int id_2bar, id_2amper, id_exclam;
	private final int id_lbracket, id_rbracket, id_lparen, id_rparen;

	private final int id_if, id_else, id_while, id_do, id_foreach, id_in;
	private final int id_return, id_try, id_catch, id_throw;
	private final int id_switch, id_case, id_default, id_break, id_continue;
	private final int id_block_open, id_block_close;

	private final int id_class, id_var, id_void, id_attribute, id_public, id_new;
	private final int id_extends, id_implements, id_interface;

	//private final int id_const;
	private final int id_summon, id_thread, id_package, id_import;


	protected int getInt() throws PlcException { return super.getInt(id_int_const); }
	protected String getString() throws PlcException { return super.getString(id_string_const); }
	protected String getIdent() throws PlcException { return super.getIdent(id_ident); }

	Node sequence( Node list, Node leaf )
	{
		if( list == null ) return leaf;
		if( leaf == null ) return list;
		return new SequenceNode( list, leaf ).setContext( l );
	}

	public Grammar(Lex l, String filename) throws PlcException {
		super(l,filename);

		// It is a default base class so we need it in any case.
		classes.do_import(".internal.object");

		try { classes.do_import(".internal.int"); } finally {}
		try { classes.do_import(".internal.string"); } finally {}

		id_at          = l.create_keyword("@");
		id_src_req     = l.create_keyword("!->");
		id_dst_req     = l.create_keyword("->!");
		id_attribute   = l.create_keyword("attribute");

		id_block_open  = l.create_keyword("{");
		id_block_close = l.create_keyword("}");

		id_plus        = l.create_keyword("+");
		id_minus       = l.create_keyword("-");
		id_slash       = l.create_keyword("/");
		id_aster       = l.create_keyword("*");
		id_percent     = l.create_keyword("%");

		id_2bar        = l.create_keyword("||");
		id_2amper      = l.create_keyword("&&");
		id_exclam      = l.create_keyword("!");

		id_bar         = l.create_keyword("|");
		id_amper       = l.create_keyword("&");
		id_caret       = l.create_keyword("^");
		id_tilde       = l.create_keyword("~");

		id_ref_eq      = l.create_keyword(":==");
		id_ref_neq     = l.create_keyword(":!=");

		id_eq          = l.create_keyword("==");
		id_neq         = l.create_keyword("!=");
		id_gt          = l.create_keyword(">");
		id_lt          = l.create_keyword("<");
		id_ge          = l.create_keyword(">=");
		id_le          = l.create_keyword("<=");
		id_assign      = l.create_keyword("=");
		id_vassign     = l.create_keyword(".=");

		id_colon       = l.create_keyword(":");
		id_point       = l.create_keyword(".");
		id_semicolon   = l.create_keyword(";");
		id_comma       = l.create_keyword(",");

		id_lbracket    = l.create_keyword("[");
		id_rbracket    = l.create_keyword("]");

		id_lparen      = l.create_keyword("(");
		id_rparen      = l.create_keyword(")");

		id_package     = l.create_keyword("package");
		id_import      = l.create_keyword("import");
		id_class       = l.create_keyword("class");
		id_interface   = l.create_keyword("interface");
		id_public      = l.create_keyword("public");
		id_extends     = l.create_keyword("extends");
		id_implements  = l.create_keyword("implements");

		id_if          = l.create_keyword("if");
		id_else        = l.create_keyword("else");
		id_while       = l.create_keyword("while");
		id_do          = l.create_keyword("do");
		id_foreach     = l.create_keyword("foreach");
		id_in          = l.create_keyword("in");
		id_return      = l.create_keyword("return");

		id_switch      = l.create_keyword("switch");
		id_case        = l.create_keyword("case");
		id_default     = l.create_keyword("default");
		id_break       = l.create_keyword("break");
		id_continue    = l.create_keyword("continue");

		id_try         = l.create_keyword("try");
		id_catch       = l.create_keyword("catch");
		id_throw       = l.create_keyword("throw");

		id_var         = l.create_keyword("var");
		id_void        = l.create_keyword("void");
		id_null        = l.create_keyword("null");
		id_new         = l.create_keyword("new");
		id_this        = l.create_keyword("this");

		id_summon      = l.create_keyword("summon");
		id_thread      = l.create_keyword("thread");

		// parametric
		id_ident = Lex.get_id();
		l.create_token(new TokenIdent(id_ident));

		id_string_const = Lex.get_id();
		l.create_token(new TokenStringConst(id_string_const));

		id_int_const = Lex.get_id();
		l.create_token(new TokenIntConst(id_int_const));
	}

	public void print() throws PlcException { classes.print(); }
	public void codegen() throws PlcException, IOException { classes.codegen(); }


	public Node parse() throws PlcException, IOException {
		Node out = null;
		while (true) {

			Token t = l.get();
			if (t.is_eof())         break; // eof

			Node top = parseTop(t);
			if( top != null ) out = sequence( out, top );
			expect( id_semicolon, "semicolon after top level definition expected");
		}

		out.preprocess(ps);

		return out;
	}

	// --------------------------------------------------------------------------
	// Top level
	// --------------------------------------------------------------------------

	Node parseTop(Token t) throws IOException, PlcException {
		if(parser_debug) System.out.println("Top level Token: " + t.toString());

		Node out = null;
		if (t.get_id() == id_class)                out = parseClass(false);
		else if (t.get_id() == id_interface)       out = parseClass(true);
		else if (t.get_id() == id_package)         out = parsePackage();
		else if (t.get_id() == id_import)          out = parseImport();
		//else if (t.get_id() == id_at)              { l.unget(); out = parse_attributes(true); }
		else if (t.get_id() == id_attribute)       { l.unget(); out = parse_attributes(true); }
		else if (t.get_id() == id_semicolon)       l.unget(); // empty op
		else       syntax_error("Unknown stuff: " + t.toString());

		return out;
	}


	private Node parsePackage() throws PlcException, IOException {
		String pname = parseClassName(true);

		if( package_name != null ) syntax_error("package is already defined as "+package_name);
		else package_name = pname;

		return null;
	}

	private Node parseImport() throws PlcException, IOException
	{
		StringBuffer sb = new StringBuffer();

		if( !testAndEat( id_point ) ) syntax_error("import must use absolute names");

		while(true)
		{
			sb.append('.');

			if( testAndEat( id_aster ) )
			{
				sb.append("*");
				break;
			}

			if( peek() == id_string_const )
				sb.append(getString());
			else
				sb.append(getIdent());
			
			if( !testAndEat( id_point ) )
				break;
		}

		if (parser_debug) System.out.println("import: " + sb.toString());

		if( !classes.do_import(sb.toString()) )
			syntax_error("Can't import " + sb.toString());

		return null;
	}


	// --------------------------------------------------------------------------
	// Class
	// --------------------------------------------------------------------------

	private Node parseClass(boolean interface_mode) throws PlcException, IOException {
		PhantomClass me = new PhantomClass(parseClassName(true));
		classes.add(me);

		Node out = null;

		parse_attributes( false );

		ps.set_class(me); // we are in class

		while(true)
		{
			int id = peek();

			if( id == id_extends )
			{
				l.get();
				String class_to_extend = parseClassName(false);
				if(interface_mode)
					syntax_error("interface can not extend class");
				if( !me.addParent(class_to_extend, ps) )
					syntax_error("just one base class (yet?)");
			}
			else if( id == id_implements )
			{
				l.get();
				String interface_to_implement = parseClassName(false);
				me.addInterface(interface_to_implement, ps);
			}
			else
				break;
		}

		expect(id_block_open);
		out = new ClassDefinitionNode( me, parseClassBody(me, interface_mode) ).setContext( l );
		expect(id_block_close);

		me.setReferencedClasses(ps.getReferencedClasses());
		
		ps.set_class(null); // came out of class

		return out;
	}


	private Node parseClassBody(PhantomClass me, boolean interface_mode) throws PlcException, IOException {
		Node out = null;

		while(true)
		{
			Token t = l.get();
			//if (parser_debug) System.out.println("class body Token: " + t.toString());

			if( t.get_id() == id_block_close )
			{
				l.unget();
				return out;
			}

			if( /*t.get_id() == id_at || */ t.get_id() == id_attribute )
			{
				l.unget();
				parse_attributes( true );
			}

			// field
			if( t.get_id() == id_var ) {
				if(interface_mode)
					syntax_error("interface can not have fields");
				PhantomType type = new PhTypeUnknown();
				String vname = getIdent();
				if(testAndEat( id_colon ))
					type = parseType();
				parse_attributes( false );
				expect( id_semicolon );
				// add var to tables

				me.addField(vname,type);
				continue;
			}
			l.unget();

			// Method or field
			if (possibleType()) {
				//int required_method_index = -1;
				PhantomType type = parseType();
				parse_attributes( false );
				String mname = getIdent();

				// if not '(' - define a variable.
				if( !peek(id_lparen) )
				{
					me.addField(mname, type);
					expect( id_semicolon );
					continue;
				}

				Method m = me.addMethod( mname, type );

				//Node args = 
				parseDefinitionArglist(m);
				if( testAndEat(id_lbracket) )
				{
					int required_method_index = getInt();					
					me.setMethodOrdinal(m,required_method_index);
					expect(id_rbracket);
				}
				parse_attributes( false );
				//Node code =
				if(interface_mode)
				{
					expect(id_semicolon);
				}
				else
				{
					ps.set_method(m);
					m.code = parseBlock();
					ps.set_method(null);
					//out = sequence( out, code ); // temp!
					//me.add_method( mname, type, args, code, required_method_index );
				}
				continue;
			}

			Token tt = l.get();
			if( t.is_eof() ) return out;
			// nothing good?
			syntax_error("Bad class element definition: "+tt.toString());
		}
	}

	private Node parseDefinitionArglist(Method m) throws PlcException, IOException {
		expect(id_lparen);
		if( testAndEat( id_rparen ) ) return null;

		Node list = null;

		while(true)
		{
			if( l.is_eof() ) { syntax_error("arg list syntax"); return null; }
			if (testAndEat( id_var ) ) {
				PhantomType type = new PhTypeUnknown();
				String vname = getIdent();
				if(testAndEat( id_colon ))
					type = parseType();
				parse_attributes( false );
				// add var to tables
				m.addArg( vname, type );

				//list = sequence( list, new arg_definition_node( vname, type ) );
				if( testAndEat( id_comma ) )
					continue;
				expect(id_rparen);
				return list;
			}
			else
			{
				syntax_error("arg list syntax");
				return null;
			}

		}
	}

	private PhantomType parseType() throws PlcException, IOException {
		Token t = l.get();
		if (parser_debug) System.out.println("type Token: " + t.toString());

		PhantomType main_type = new PhTypeUnknown();

		if(t.get_id() == id_void ) main_type = new PhTypeVoid();
		else if(t.get_id() == id_aster )
		{
			// *type-var case
			main_type._class_expression = parseExpression(false);
		}
		else {
			l.unget();
			String cln = parseClassName(false);

			if( cln.equals("int") ) main_type = new PhTypeInt();
			else if( cln.equals("string") ) main_type = new PhTypeString();
			else
			{
				PhantomClass c = classes.get(cln, false, ps);
				if (c == null)
					syntax_error("Class " + cln + " is not defined - forgot to import?");

				main_type = new PhantomType(c);
			}
		}

		// here we'll look for [] stuff

		if( testAndEat( id_lbracket ) )
		{
			if( testAndEat( id_rbracket ) )
			{
				main_type._is_container = true;
			}
			else
			{
				main_type._is_container = true;

				if( testAndEat( id_aster ) )
					main_type._container_class_expression = parseExpression(false);
				else
					main_type._container_class = new PhantomClass(parseClassName(false));

				expect( id_rbracket );
			}
		}

		// atrrs
		parse_attributes( false );

		return main_type;
	}


	private boolean possibleType() throws PlcException, IOException {
		int id = peek();
		return ( id == id_aster || id == id_point || id == id_void || id == id_ident );
	}


	private String parseClassName(boolean definition)  throws PlcException, IOException {
		StringBuffer sb = new StringBuffer();
		boolean absolute = false;

		if( testAndEat( id_point ) )  {
			absolute = true;
			//sb.append(".");
		}

		boolean first = true;

		while(true)
		{
			sb.append('.');
			String is;

			int id = peek();
			if( id == id_string_const)
			{
				is = getString();
			}
			else
				is = getIdent();
			if( !absolute && first && is.equals("int") ) return "int";
			if( !absolute && first && is.equals("string") ) return "string";
			sb.append(is);
			first = false;
			if( !testAndEat( id_point ) )
				break;
		}

		if( definition && (!absolute) )
		{
			String pname = package_name;
			if( pname == null )
			{
				syntax_error("Relative name in definition, but no package was defined");
				pname = ".error.package";
			}

			if (parser_debug) System.out.println("def rel class name: " + pname+sb.toString());
			return pname+sb.toString();
		}

		if( (!definition) && (!absolute) )
		{
			// TODO Here we need to scan through 'uses' tables and find target
			String pname = package_name;
			if( pname == null )
			{
				syntax_error("Relative name in reference, but no package was defined");
				pname = ".error.package";
			}

			if (parser_debug) System.out.println("ref rel class name: " + pname+sb.toString());
			return pname+sb.toString();
		}

		if (parser_debug) System.out.println("abs class name: " + sb.toString());
		return sb.toString();
	}

	// --------------------------------------------------------------------------
	// Block
	// --------------------------------------------------------------------------

	private Node parseBlock() throws PlcException, IOException {
		Node out = null;
		expect(id_block_open);

		while(true)
		{
			if( l.is_eof() )
			{
				syntax_error("unexpected EOF in block");
				return null;
			}

			int id = peek();

			if (id == id_block_close)     { l.get(); break; }
			else                          out = sequence( out, parseOperator() );
		}

		return out;
	}

	// --------------------------------------------------------------------------
	// Flow control, code  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// --------------------------------------------------------------------------

	private Node parseOperator() throws PlcException, IOException {
		// this will break expressions. :(
		/*if (possibleType()) {
			PhantomType type = parseType();
			parse_attributes( false );
			String mname = get_ident();

			// if not '(' - define a variable.
			if( !peek(id_lparen) )
			{
				ps.get_method().svars.add_stack_var(new PhantomVariable(mname, type));

				expect( id_semicolon );
				return null;
			}
			else
			{
				syntax_error("nested functions are not yet.");
				return null;
			}
		}*/

		int id = peek();

		if (id == id_do)              return parseDo();
		else if (id == id_while)      return parseWhile();
		else if (id == id_if)         return parseIf();
		else if (id == id_foreach)    return parseForeach();
		else if (id == id_return)     return parseReturn();
		else if (id == id_throw)      return parseThrow();
		else if (id == id_try)        return parseTry();
		else if (id == id_switch)     return parseSwitch();
		else if (id == id_break)      return parseBreak();
		else if (id == id_continue)   return parseContinue();
		else if (id == id_block_open) return parseBlock();
		else if (id == id_var)        return parseAutoVarDefinition();
		else
		{
			Node expr = parseExpression(true);
			expect(id_semicolon);
			if( expr == null )
			{
				syntax_error("syntax error in Method body");
				l.get(); // eat some bad stuff...
			}
			
			return new VoidNode( expr );
		}
	}


	private Node parseAutoVarDefinition() throws PlcException, IOException, PlcException {
		expect(id_var);
		String ident = getIdent();
		expect(id_colon);
		PhantomType t = parseType();
		if( t.is_int() )
			ps.get_method().isvars.add_stack_var(new PhantomVariable(ident, t));
		else
			ps.get_method().svars.add_stack_var(new PhantomVariable(ident, t));
		expect( id_semicolon);
		return null;
	}


	private Node parseBreak() throws PlcException, IOException {
		expect( id_break );
		expect( id_semicolon );
		return new BreakNode().setContext( l );
	}

	private Node parseContinue() throws PlcException, IOException {
		expect( id_continue );
		expect( id_semicolon );
		return new ContinueNode().setContext( l );
	}


	private Node parseSwitch() throws PlcException, IOException {
		expect(id_switch);
		expect(id_lparen);
		Node expr = parseExpression(false);
		expect(id_rparen);

		SwitchNode sw_node = new SwitchNode(expr);
		sw_node.setContext( l );
		Node code = null;

		expect(id_block_open);

		while(true)
		{
			int id = peek();

			if(l.is_eof())
			{
				syntax_error("unexpected EOF in switch");
				return null;
			}
			else if (id == id_case)
			{
				l.get();
				String case_label = ps.get_method().get_cg().getLabel();
				SwitchCaseNode case_n = new SwitchCaseNode( case_label, parseExpression(false) );
				case_n.setContext( l );
				code = sequence( code, case_n );
				sw_node.add_case(case_n);
				expect( id_colon );
			}
			else if (id == id_default)
			{
				l.get();
				String default_label = ps.get_method().get_cg().getLabel();
				SwitchDefaultNode default_n = new SwitchDefaultNode(default_label);
				default_n.setContext( l );
				code = sequence( code, default_n );
				sw_node.add_default(default_n);
				expect( id_colon );
			}
			else if (id == id_block_close)     { l.get(); break; }
			else                               code = sequence( code, parseOperator() );
		}

		code = sequence( code, new BreakNode().setContext( l ) );
		
		sw_node.add_code(code);
		return sw_node;
	}


	private Node parseTry() throws PlcException, IOException {
		expect(id_try);
		Node code = parseOperator();

		expect(id_catch);

		expect(id_lparen);
		PhantomType t = parseType();
		String id_name = getIdent();
		expect(id_rparen);
		Node catcher = parseOperator();

		ps.get_method().svars.add_stack_var(new PhantomVariable(id_name,t));

		return new CatchNode( t, id_name, code, catcher ).setContext( l );
	}


	private Node parseThrow() throws PlcException, IOException {
		expect(id_throw);
		Node expr = parseExpression(false);
		expect( id_semicolon );
		return new ThrowNode(expr).setContext( l );
	}


	private Node parseDo() throws PlcException, IOException {
		expect(id_do);
		Node pre = parseOperator();
		expect(id_while);
		expect(id_lparen);
		Node expr = parseExpression(false);
		expect(id_rparen);
		if( testAndEat( id_semicolon ) )
			return new DoWhileNode(pre, expr, null ).setContext( l );
		return new DoWhileNode(pre, expr, parseOperator() ).setContext( l );
	}

	private Node parseWhile() throws PlcException, IOException {
		expect(id_while);
		expect(id_lparen);
		Node expr = parseExpression(false);
		expect(id_rparen);
		return new DoWhileNode(null, expr, parseOperator() ).setContext( l );
	}

	private Node parseReturn() throws PlcException, IOException {
		expect(id_return);
		if( testAndEat( id_semicolon ) ) return new ReturnNode( null ).setContext( l );
		Node expr = parseExpression(false);
		expect( id_semicolon );
		return new ReturnNode( expr ).setContext( l );
	}

	private Node parseIf() throws PlcException, IOException {
		expect(id_if);
		expect(id_lparen);
		Node expr = parseExpression(false);
		expect(id_rparen);
		Node block1 = parseOperator();
		if( !testAndEat( id_else ) )
			return new IfNode( expr, block1, null ).setContext( l );
		Node block2 = parseOperator();
		return new IfNode( expr, block1, block2 ).setContext( l );
	}

	private Node parseForeach() throws PlcException, IOException {
		expect( id_foreach );
		expect( id_lparen );
		Token ident = expect( id_ident );
		expect( id_in );
		Node expr = parseExpression(false);
		expect( id_rparen );
		Node code = parseOperator();

		// TODO Must have type of expression!
		ps.get_method().svars.add_stack_var(new PhantomVariable(ident.value(),new PhTypeUnknown()));

		// secondary (internal) var to keep iterator
		ps.get_method().svars.add_stack_var(new PhantomVariable(ident.value()+"$iterator",new PhTypeUnknown()));

		return new ForeachNode( ident.value(), expr, code ).setContext( l );
	}

	// --------------------------------------------------------------------------
	// Expression
	// --------------------------------------------------------------------------

	/** 
	 * @param leftmost True if this is a good place to look for var definition.
	 */
	private Node parseExpression(boolean leftmost) throws PlcException, IOException {
		return parseRvalue(leftmost);
	}

	private Node parseRvalue(boolean leftmost) throws PlcException, IOException {
		return parseLogical(leftmost);
	}


	private Node parse_method_id() throws PlcException, IOException {
		Token t = l.get();
		int id = t.get_id();

		if( id == id_ident )      return new MethodNode(t.value()).setContext( l );
		else if( id == id_int_const )
		{
			l.unget();
			return new MethodNode(getInt()).setContext( l );
		}
		else
		{
			l.unget();
			expect( id_lparen ); Node expr = parseExpression(false); expect( id_rparen );
			return expr;
		}
	}




	private Node parse_call_args() throws PlcException, IOException {
		expect(id_lparen);
		return parse_call_arg();
	}

	private Node parse_call_arg() throws PlcException, IOException {
		if( l.is_eof() )
		{
			syntax_error("unexpected EOF in arglist");
			return null;
		}
		if( testAndEat( id_rparen ) ) return null;
		testAndEat( id_comma );
		Node expr = parseExpression(false);
		return new CallArgNode( expr, parse_call_arg() ).setContext( l );
	}

	// --------------------------------------------------------------------------
	// Expression - ops
	// --------------------------------------------------------------------------




	private Node parseLogical(boolean leftmost) throws PlcException, IOException {

		if( testAndEat(id_exclam) )
			return new BoolNotNode( parseLogical(false) ).setContext( l );

		Node out = parseCmp(leftmost);
		if( out == null ) return null;

		while(true)
		{
			int id = l.get().get_id();
			if( id == id_2bar )          out = new BoolOrNode( out, parseCmp(false) ).setContext( l );
			else if( id == id_2amper )   out = new BoolAndNode( out, parseCmp(false) ).setContext( l );
			//else if( id == id_exclam )   out = new bool_not_node( out );
			else                         { l.unget(); break; }
		}
		return out;
	}


	private Node parseCmp(boolean leftmost) throws PlcException, IOException {
		Node out = parseRefCmp(leftmost);
		if( out == null ) return null;

		while(true)
		{
			Token t = l.get();
			int id = t.get_id();
			if( id == id_eq )        out = new ValEqNode( out, parseRefCmp(false) ).setContext( l );
			else if( id == id_neq )  out = new ValNeqNode( out, parseRefCmp(false) ).setContext( l );
			else if( id == id_gt )   out = new ValGtNode( out, parseRefCmp(false) ).setContext( l );
			else if( id == id_lt )   out = new ValLtNode( out, parseRefCmp(false) ).setContext( l );
			else if( id == id_ge )   out = new ValGeNode( out, parseRefCmp(false) ).setContext( l );
			else if( id == id_le )   out = new ValLeNode( out, parseRefCmp(false) ).setContext( l );
			else                     { l.unget(); break;  }
		}
		return out;
	}

	private Node parseRefCmp(boolean leftmost) throws PlcException, IOException {
		Node out = parseAdditive(leftmost);
		if( out == null ) return null;

		while(true)
		{
			Token t = l.get();
			int id = t.get_id();
			if( id == id_ref_eq )       out = new RefEqNode( out, parseAdditive(false) ).setContext( l );
			else if( id == id_ref_neq ) out = new RefNeqNode( out, parseAdditive(false) ).setContext( l );
			else                     { l.unget(); break; }
		}
		return out;
	}

	private Node parseAdditive(boolean leftmost) throws PlcException, IOException {
		Node out = parseMult(leftmost);
		if( out == null ) return null;

		while(true)
		{
			Token t = l.get();
			int id = t.get_id();
			if( id == id_plus )        out = new OpPlusNode( out, parseMult(false) ).setContext( l );
			else if( id == id_minus )       out = new OpMinusNode( out, parseMult(false) ).setContext( l );
			else                       { l.unget(); break; }
		}
		return out;
	}

	private Node parseMult(boolean leftmost) throws PlcException, IOException {
		Node out = parseBit(leftmost);
		if( out == null ) return null;

		while(true)
		{
			int id = l.get().get_id();
			if( id == id_slash )       out = new OpDivideNode( out, parseBit(false) ).setContext( l );
			else if( id == id_aster )  out = new OpMultiplyNode( out, parseBit(false) ).setContext( l );
            else if( id == id_percent )out = new OpRemainderNode( out, parseBit(false) ).setContext( l );
			else                        { l.unget(); break; }
		}
		return out;
	}

	private Node parseBit(boolean leftmost) throws PlcException, IOException {
		Node out = parseLower(leftmost);
		if( out == null ) return null;

		while(true)
		{
			Token t = l.get();
			int id = t.get_id();
			if( id == id_bar )         out = new OpOrNode( out, parseLower(false) ).setContext( l );
			else if( id == id_amper )  out = new OpAndNode( out, parseLower(false) ).setContext( l );
			else if( id == id_tilde )  out = new OpNotNode( out ).setContext( l );
			else                       { l.unget(); break;  }
		}
		return out;
	}

	private Node parseLower(boolean leftmost) throws PlcException, IOException {
		Token t = l.get();
		int id = t.get_id();

		if(id == id_string_const )
		{
			return new StringConstNode(t.value()).setContext( l );
		}
		else if (t.get_id() == id_import)          
		{
			byte [] data = parseStringImport();
			return new BinaryConstNode(data).setContext( l );
		}		
		else if(id == id_int_const )
		{
			TokenIntConstVal ti = (TokenIntConstVal)t;
			return new IntConstNode(ti.int_value()).setContext( l );
		}
		else if(id == id_new )
		{
			l.unget();
			return parse_new();
		}
		else
		{
			l.unget();
			return parseLvalue(leftmost);
		}
	}

	private byte [] parseStringImport() throws PlcException
	{
		ByteArrayOutputStream ba = new ByteArrayOutputStream();

		Token t = l.get();
		int id = t.get_id();

		if(id == id_string_const )
		{
			String fname = t.value();
			try {
				FileInputStream fis = new FileInputStream(fname);

				byte[] buf = new byte[1024];
				int i = 0;
				while ((i = fis.read(buf)) != -1) {
					ba.write(buf, 0, i);
				}
				fis.close();
			} catch (FileNotFoundException e) {
				syntax_error("File '"+fname+"' not found or can't be read");
			} catch( IOException e ) {
				syntax_error("File '"+fname+"': IO Error");
			}
		}
		else
		{
			l.unget();
			syntax_error("File name (string const) expected, but found " + t.toString());
		}
		try {
			ba.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return ba.toByteArray();
	}

	// var, Method call, array subscription, assignment
	private Node parseLvalue(boolean leftmost) throws PlcException, IOException
	{
		Node atom = parseAtom(leftmost);
		Node out = atom;
		if( atom == null )
		{
			// TODO when implementing local var C/Java declaration syntax this must not be an error
			// as we return null in case of declaration is found in parseAtom
			syntax_error("expression syntax");
			l.get();
			return null;
			//return parse_rvalue();
		}

		while(true)
		{
			if (testAndEat( id_point )) {
				Node method = parse_method_id();
				Node args = parse_call_args();
				//out = new op_method_call_node(atom, method, args).setContext( l );
				out = new OpMethodCallNode(out, method, args).setContext( l );
			}
			else if (peek( id_lparen )) {
				// TODO this is wrong and can't be in loop? What if we already have out?
				
				if(out != atom)
					syntax_error("call of non-method?");
				
				Node object = new ThisNode(ps.get_class()).setContext( l );
				Node args = parse_call_args();

				IdentNode ident = (IdentNode)atom;
				Node method = new MethodNode( ident.get_name() ).setContext( l );
				out = new OpMethodCallNode(object, method, args).setContext( l );
				//out = new op_method_call_node(object, atom, args);
			}
			else if (testAndEat( id_lbracket )) {
				Node subscript = parseExpression(false);
				//out = new OpSubscriptNode( atom, subscript ).setContext( l );
				out = new OpSubscriptNode( out, subscript ).setContext( l );
				expect( id_rbracket );
			}
			else
				break;
		}

		{
			Token t = l.get();
			int id = t.get_id();
			if (id == id_assign) out = new OpAssignNode(out, parseExpression(false)).setContext( l );
			else if (id == id_vassign) out = new OpVassignNode(out,
					parseExpression(false)).setContext( l );
			else l.unget();
		}

		return out;
	}

	// identifier or expression in ()s
	private Node parseAtom(boolean leftmost) throws PlcException, IOException {
		Token t = l.get();
		int id = t.get_id();

		if( id == id_ident )
		{
			// TODO check for var def with leading type
			if( leftmost) {
				PhantomType type = null;

				String typeName = t.value();
				if(typeName.equals("int")) { typeName = ".internal.int"; type = new PhTypeInt(); }
				else if(typeName.equals("string")) { typeName = ".internal.string"; type = new PhTypeString(); }
				else
				{
					ClassMap.get_map().do_import(typeName);
					PhantomClass phantomClass = ClassMap.get_map().get(typeName, true, ps);
					if( phantomClass != null )
						type = new PhantomType( phantomClass );
				}

				if(type != null)
				{
					syntax_warning("Type name - var def??");

					parse_attributes( false );
					String mname = getIdent();
					//expect(id_semicolon);


					ps.get_method().svars.add_stack_var(new PhantomVariable(mname, type));
					return new EmptyNode();
				}
			} 

			return new IdentNode( t.value() ).setContext( l );
		}
		else if(id == id_null)
			return new NullNode().setContext( l );
		else if(id == id_this)
			return new ThisNode(ps.get_class()).setContext( l );
		else if(id == id_lparen)
		{
			Node n = parseExpression(false);
			expect( id_rparen );
			return n;
		}
		else
		{
			l.unget();
			// TODO return error message here?
			//syntax_error("expression syntax error, unexpected: "+t.toString() );
			return null;
		}
	}

	private Node parse_new() throws PlcException, IOException {
		expect( id_new );
		Node type_expr = null;
		PhantomType type = null;

		if( testAndEat( id_aster ) )
		{
			expect(id_lparen);
			type_expr = parseExpression(false);
			expect(id_rparen);
		}
		else                             type = parseType();

		Node args = parse_call_args();

		return new NewNode(type,type_expr,args).setContext( l );
	}

	// --------------------------------------------------------------------------
	// Helpers - attributes
	// --------------------------------------------------------------------------

	private Node parse_attributes( boolean definition ) throws PlcException, IOException {
		int id = peek();
		if( id != id_at && id != id_attribute ) return null;
		l.get(); // eat @/attribute

		do { parse_attribute( definition ); } while( testAndEat( id_comma ) );
		return null;
	}

	private void parse_attribute(boolean definition) throws PlcException, IOException
	{
		boolean is_additive = false, is_multiplicative = false;
		boolean is_required = false, is_negation = false;
		boolean is_src_req = false, is_dst_req = false;

		if( testAndEat( id_tilde ) ) is_negation = true;

		String aname = parseClassName(definition);

		while(true)
		{
			int id = l.get().get_id();
			if( l.is_eof() ) throw new PlcException("parse_attribute", "unexpected eof");

			if( id == id_plus )          is_additive = true;
			else if( id == id_aster )    is_multiplicative = true;
			else if( id == id_src_req )  is_src_req = true;
			else if( id == id_dst_req )  is_dst_req = true;
			else if( id == id_exclam )   is_required = true;
			else break;
		}
		l.unget();

		if(is_multiplicative && is_additive)
			syntax_error("Attribute "+aname+" defined as multiplicative and additive. Kidding?");

		if( definition )
		{
			if(is_negation)
				syntax_error("Attribute "+aname+" has negation in definition");
		}
		else
		{
			if( is_src_req || is_dst_req )
				syntax_error("Attribute "+aname+" has partial requirements (!-> or ->!) not in definition");
		}

	}

}




