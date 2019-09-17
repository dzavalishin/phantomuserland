package ru.dz.phantasm;

import java.io.*;

import ru.dz.phantom.code.*;
import ru.dz.plc.parser.*;
import ru.dz.plc.util.*;

import java.util.*;

public class grammar
  extends GrammarHelper {
  protected Codegen cg;

  private final int id_colon, id_semicolon;
  private final int id_plus, id_minus, id_slash, id_aster;
  private final int id_ior, id_ixor, id_iand, id_inot;
  private final int id_bar, id_caret, id_amper, id_tilde;
  private final int id_2bar, id_2amper, id_exclam;
  private final int id_colon_eq;

  private final int id_const;
  private final int id_nop, id_drop, id_dup;
  private final int id_is, id_os, id_cs;

  private final int id_jmp, id_djnz, id_jz;
  private final int /*id_skipz, id_skipnz, */ id_switch;

  private final int id_ret, id_call;

  private final int id_summon, id_thread, id_this, id_debug;

  private final int id_i20, id_o2i;
  private final int id_isum, id_imul;
  private final int id_isubul, id_isublu, id_idivul, id_idivlu;

  private final int id_load, id_save, id_new, id_copy;
  private final int id_compose, id_decompose;

  private final int id_push, id_pop, id_pull, id_catcher, id_throw;

  protected final int id_ident, id_string_const, id_int_const;





  public grammar(Lex l, String fname, Codegen cg) throws PlcException {
    super(l,fname);
    //this.l = l;
    this.cg = cg;

    id_const     = l.create_keyword("const");

    id_nop       = l.create_keyword("nop");

    id_is        = l.create_keyword("is"); // integer stack
    id_os        = l.create_keyword("os"); // object stack
    id_cs        = l.create_keyword("cs"); // code (return) stack

    id_i20       = l.create_keyword("i2o");
    id_o2i       = l.create_keyword("o2i");
    id_isum      = l.create_keyword("isum");
    id_imul      = l.create_keyword("imul");

    id_isubul    = l.create_keyword("isubul");
    id_isublu    = l.create_keyword("isublu");
    id_idivul    = l.create_keyword("idivul");
    id_idivlu    = l.create_keyword("idivlu");

    id_iand      = l.create_keyword("iand");
    id_ior       = l.create_keyword("ior");
    id_inot      = l.create_keyword("inot");
    id_ixor      = l.create_keyword("ixor");

    id_drop      = l.create_keyword("drop");
    id_dup       = l.create_keyword("dup");
    id_pull      = l.create_keyword("pull");

    id_jmp       = l.create_keyword("jmp");
    id_djnz      = l.create_keyword("djnz");
    id_jz        = l.create_keyword("jz");
    //id_skipz     = l.create_keyword("skipz");
    //id_skipnz    = l.create_keyword("skipnz");
    id_switch    = l.create_keyword("switch");

    id_ret       = l.create_keyword("ret");
    id_call      = l.create_keyword("call");

    id_summon    = l.create_keyword("summon");
    id_thread    = l.create_keyword("thread");
    id_this      = l.create_keyword("this");
    id_debug     = l.create_keyword("debug");

    id_load      = l.create_keyword("load");
    id_save      = l.create_keyword("save");
    id_new       = l.create_keyword("new");
    id_copy      = l.create_keyword("copy");

    id_compose   = l.create_keyword("compose");
    id_decompose = l.create_keyword("decompose");

    id_push      = l.create_keyword("push");
    id_pop       = l.create_keyword("pop");
    id_catcher   = l.create_keyword("catcher");
    id_throw     = l.create_keyword("throw");

    id_colon     = l.create_keyword(":");
    id_semicolon = l.create_keyword(";");

    id_plus      = l.create_keyword("+");
    id_minus     = l.create_keyword("-");
    id_slash     = l.create_keyword("/");
    id_aster     = l.create_keyword("*");

    id_2bar       = l.create_keyword("||");
    id_2amper     = l.create_keyword("&&");
    id_exclam     = l.create_keyword("!");

    id_bar       = l.create_keyword("|");
    id_amper     = l.create_keyword("&");
    id_caret     = l.create_keyword("^");
    id_tilde     = l.create_keyword("~");

    id_colon_eq  = l.create_keyword(":=");

    // parametric
    id_ident = Lex.get_id();
    l.create_token(new TokenIdent(id_ident));

    id_string_const = Lex.get_id();
    l.create_token(new TokenStringConst(id_string_const));

    id_int_const = Lex.get_id();
    l.create_token(new TokenIntConst(id_int_const));
  }

  protected int get_int() throws PlcException { return super.getInt(id_int_const); }
  protected String get_string() throws PlcException { return super.getString(id_string_const); }
  protected String get_ident() throws PlcException { return super.getIdent(id_ident); }


  public void parse() throws PlcException, IOException {
    while (true) {
      Token t = l.get();
      if (t.is_eof()) {
        break; // eof
      }
      parse_top(t);

      if (l.get().get_id() != id_semicolon) {
        l.unget();
        syntax_warning("no semicolon after operator");
      }
    }
  }

  void parse_top(Token t) throws IOException, PlcException {
    System.out.println("Top level Token: " + t.toString());

    if (t.get_id() == id_ident) {
      parse_label(t);
    }
    else if (t.get_id() == id_semicolon) {
      l.unget(); // empty op
    }
    else if (t.get_id() == id_nop)              cg.emitNop();
    else if (t.get_id() == id_jmp)              parse_jmp();
    else if (t.get_id() == id_debug)            parse_debug();
    else if (t.get_id() == id_djnz)             parse_djnz();
    else if (t.get_id() == id_jz)               parse_jz();
    //else if (t.get_id() == id_skipz)            cg.emit_skipz();
    //else if (t.get_id() == id_skipnz)           cg.emit_skipnz();
    else if (t.get_id() == id_switch)           parse_switch();
    else if (t.get_id() == id_call)             parse_call();
    else if (t.get_id() == id_ret)              cg.emitRet();
    else if (t.get_id() == id_is)               parse_is();
    else if (t.get_id() == id_os)               parse_os();
    else if (t.get_id() == id_cs)               parse_cs();

    else if (t.get_id() == id_load)             parse_load();
    else if (t.get_id() == id_save)             parse_save();
    else if (t.get_id() == id_new)              cg.emitNew();
    else if (t.get_id() == id_copy)             cg.emitCopy();
    else if (t.get_id() == id_colon_eq)         cg.emitCopy();

    else if (t.get_id() == id_compose )         cg.emit_compose(get_int());
    else if (t.get_id() == id_decompose )       cg.emit_decompose();
    else if (t.get_id() == id_i20)              cg.emit_i2o();
    else if (t.get_id() == id_o2i)              cg.emit_o2i();
    else if (t.get_id() == id_isum)             cg.emitISum();
    else if (t.get_id() == id_imul)             cg.emitIMul();
    else if (t.get_id() == id_isubul)           cg.emitISubUL();
    else if (t.get_id() == id_isublu)           cg.emitISubLU();
    else if (t.get_id() == id_idivul)           cg.emitIDivUL();
    else if (t.get_id() == id_idivlu)           cg.emitIDivLU();

    // bitwise
    else if (t.get_id() == id_ior  || t.get_id() == id_bar   ) cg.emit_ior();
    else if (t.get_id() == id_iand || t.get_id() == id_amper ) cg.emit_iand();
    else if (t.get_id() == id_ixor || t.get_id() == id_caret ) cg.emit_ixor();
    else if (t.get_id() == id_inot || t.get_id() == id_tilde ) cg.emit_ior();

    // logical
    else if (t.get_id() == id_2bar )            cg.emitLogOr();
    else if (t.get_id() == id_2amper )          cg.emitLogAnd();
    else if (t.get_id() == id_exclam )          cg.emitLogNot();

    // shortcuts for int stack
    else if (t.get_id() == id_plus)             cg.emitISum();
    else if (t.get_id() == id_minus)            cg.emitISubUL();
    else if (t.get_id() == id_aster)            cg.emitIMul();
    else if (t.get_id() == id_slash)            cg.emitIDivUL();

    // exceptions
    else if (t.get_id() == id_push)             parse_push();
    else if (t.get_id() == id_pop)              parse_pop();
    else if (t.get_id() == id_throw)            cg.emitThrow();

    else if (t.get_id() == id_const)            parse_const();
    else if (t.get_id() == id_int_const) {
      l.unget();
      parse_const();
    }
    else if (t.get_id() == id_string_const) {
      l.unget();
      parse_const();
    }
    else if (t.get_id() == id_summon)           parse_summon();
    else {
      syntax_error("Unknown stuff: " + t.toString());
    }
  }

  private void parse_djnz() throws PlcException, IOException {
    Token nt = l.get();
    if (nt.get_id() == id_ident) {
      cg.emitDjnz(nt.value());
    }
    else {
      l.unget();
      syntax_error("no ident after djnz");
    }
  }

  private void parse_jz() throws PlcException, IOException {
    Token nt = l.get();
    if (nt.get_id() == id_ident) {
      cg.emitJz(nt.value());
    }
    else {
      l.unget();
      syntax_error("no ident after jz");
    }
  }



  /**
   * parse_jmp
   *
   * @param t Token
   */
  private void parse_jmp() throws PlcException, IOException {
    Token nt = l.get();
    if (nt.get_id() == id_ident) {
      //cg.put_byte((byte)0x00);// ERROR! Jump op code here
      //cg.put_named_int32_reference(nt.value());
      cg.emitJmp(nt.value());
    }
    else {
      l.unget();
      syntax_error("no ident after jmp");
    }
  }

private void parse_switch() throws PlcException, IOException {

  int shift = get_int();
  int divisor = get_int();


  LinkedList<String> table = new LinkedList<String>();
  while(true)
  {
    Token nt = l.get();
    if (nt.get_id() == id_semicolon) {
      l.unget();
      break;
    }

    if( nt.get_id() != id_ident )
    {
      syntax_error("not a label in switch");
      l.unget();
      return;
    }

    table.add( nt.value() );

  }

  cg.emitSwitch( table, shift, divisor);
  }


  /**
   * parse_label
   *
   * @param t Token
   */
  private void parse_label(Token t) throws PlcException, IOException {
    Token nt = l.get();
    if (nt.get_id() == id_colon) {
      cg.markLabel(t.value());
    }
    else {
      l.unget();
      syntax_warning("no colon after label");
      // still treat as label...
      cg.markLabel(t.value());
    }
    nt = l.get();
    if (nt.is_eof()) {
      return;
    }
    parse_top(nt);
  }

  private void parse_is() throws PlcException, IOException {
    Token nt = l.get();
    if (nt.get_id() == id_dup) {
      cg.emitIsDup();
    }
    else if (nt.get_id() == id_drop) {
      cg.emitIsDrop();
    }
    else {
      l.unget();
      syntax_error("unknown integer stack op");
    }
  }

  private void parse_os() throws PlcException, IOException {
    Token nt = l.get();
    if (nt.get_id() == id_dup) {
      cg.emitOsDup();
    }
    else if (nt.get_id() == id_drop) {
      cg.emitOsDrop();
    }
    else if (nt.get_id() == id_pull ) {
      syntax_warning("pull operation is deprecated!");
      cg.emit_pull(get_int());
    }
    else {
      l.unget();
      syntax_error("unknown object stack op");
    }
  }

  private void parse_cs() throws PlcException, IOException {
    Token nt = l.get();
    //if( nt.get_id() == id_dup )           cg.emit_os_dup();
    //else if( nt.get_id() == id_drop )     cg.emit_os_drop();
    //else
    {
      l.unget();
      syntax_error("unknown code stack op");
    }
  }

  private void parse_const() throws PlcException, IOException {
    Token nt = l.get();
    if (nt.get_id() == id_int_const) {
      TokenIntConstVal iv = (TokenIntConstVal) nt;

      if (iv.int_value() == 0) {
        cg.emitIConst_0();
      }
      else if (iv.int_value() == 1) {
        cg.emitIConst_1();
      }
      else if (iv.int_value() < Byte.MAX_VALUE &&
               iv.int_value() < Byte.MIN_VALUE) {
        cg.emitIConst_8bit( (byte) iv.int_value());
      }
      else {
        cg.emitIConst_32bit(iv.int_value());
      }
    }
    else if (nt.get_id() == id_string_const) {
      cg.emitString(nt.value());
    }
    else {
      l.unget();
      syntax_error("unknown const op " + nt.toString());
    }
  }


  void parse_summon() throws PlcException, IOException {
    Token nt = l.get();
    if (nt.get_id() == id_thread) {
      cg.emitSummonThread();
    }
    else if (nt.get_id() == id_this) {
      cg.emitSummonThis();
    }

    else {
      l.unget();
      //syntax_error("unknown summon parameter " + nt.toString());
      String name = get_string();
      cg.emitSummonByName(name);
    }

  }

  void parse_debug() throws PlcException, IOException
  {
    int type = 0;
    String text = null;
    Token nt = l.get();

    if( nt.get_id() == id_int_const )
    {
      type = ((TokenIntConstVal)nt).int_value();
      nt = l.get();
    }

    if( nt.get_id() == id_string_const )
    {
      text = nt.value();
      nt = l.get();
    }


    if( nt.get_id() != id_semicolon)
      syntax_error("unknown debug parameter " + nt.toString());

    l.unget();

    cg.emitDebug( (byte)type, text );
  }


  void parse_call() throws PlcException, IOException
  {
    int method_index = 0;
    int n_param = 0; // not impl
    Token nt = l.get();

    if( nt.get_id() == id_int_const )
    {
      method_index = ((TokenIntConstVal)nt).int_value();
      nt = l.get();
    }
    else
    {
        syntax_error("no Method index in call, but" + nt.toString());
        l.unget();
    }

    if( nt.get_id() == id_int_const )
    {
      n_param = ((TokenIntConstVal)nt).int_value();
      nt = l.get();
    }

    if( nt.get_id() != id_semicolon)
      syntax_error("unknown call parameter " + nt.toString());

    l.unget();

    cg.emitCall( method_index, n_param );
  }


  void parse_load()  throws PlcException, IOException
  {
    int slot = 0;
    Token nt = l.get();
    if( nt.get_id() == id_int_const )
      slot = ((TokenIntConstVal)nt).int_value();
    else
    {
        syntax_error("no slot index in load, but" + nt.toString());
        l.unget();
    }

    cg.emitLoad(slot);
  }

  void parse_save()  throws PlcException, IOException
  {
    int slot = 0;
    Token nt = l.get();
    if( nt.get_id() == id_int_const )
      slot = ((TokenIntConstVal)nt).int_value();
    else
    {
        syntax_error("no slot index in save, but" + nt.toString());
        l.unget();
    }

    cg.emitSave(slot);
  }

  void parse_push()  throws PlcException, IOException
  {
    expect(id_catcher);
    String label = get_ident();
    cg.emitPushCatcher(label);
  }

  void parse_pop()  throws PlcException, IOException
  {
    expect(id_catcher);
    cg.emitPopCatcher();
  }

}
