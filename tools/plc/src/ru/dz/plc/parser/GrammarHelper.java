package ru.dz.plc.parser;

import ru.dz.plc.util.*;

/**
 * <p>Parser tools.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class GrammarHelper extends GenericGrammarHelper {



	public GrammarHelper( ILex l, String fname ) throws PlcException 
	{
		super( l, fname );
		
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
		id_static      = l.create_keyword("static");
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

		//id_summon      = l.create_keyword("summon");
		//id_thread      = l.create_keyword("thread");

		/*
		// parametric
		id_ident = Lex.get_id();
		l.create_token(new TokenIdent(id_ident));

		id_string_const = Lex.get_id();
		l.create_token(new TokenStringConst(id_string_const));

		id_int_const = Lex.get_id();
		l.create_token(new TokenIntConst(id_int_const));
		*/
		initGeneralIds();
	}

	/*
	 * public GrammarHelper(Lex l, Codegen cg) throws PlcException { this.l = l;
	 * this.cg = cg; }
	 */


	/*
	 * protected void syntax_error(String warning_text, String track ) throws
	 * PlcException { System.out.print("Error: " + warning_text);
	 * System.out.println(); error_count++; System.out.println(track); if
	 * (error_count > 20) { throw new PlcException("Grammar",
	 * "too many errors"); } }
	 */



	//protected final int id_ident, id_string_const, id_int_const, id_this, id_null;
	protected final int id_this, id_null;

	protected final int id_point, id_comma, id_at, id_src_req, id_dst_req;
	protected final int id_colon, id_semicolon, id_assign, id_vassign;
	protected final int id_eq, id_neq, id_gt, id_lt, id_ge, id_le;

	protected final int id_ref_eq, id_ref_neq;
	protected final int id_plus, id_minus, id_slash, id_aster, id_percent;
	protected final int id_bar, id_caret, id_amper, id_tilde;
	protected final int id_2bar, id_2amper, id_exclam;
	protected final int id_lbracket, id_rbracket, id_lparen, id_rparen;

	protected final int id_if, id_else, id_while, id_do, id_foreach, id_in;
	protected final int id_return, id_try, id_catch, id_throw;
	protected final int id_switch, id_case, id_default, id_break, id_continue;
	protected final int id_block_open, id_block_close;

	protected final int id_class, id_var, id_void, id_attribute, id_public, id_new;
	protected final int id_extends, id_implements, id_interface, id_static;

	//protected final int id_const;
	//protected final int id_summon, id_thread, id_package, id_import;
	protected final int id_package, id_import;
	
	
}
