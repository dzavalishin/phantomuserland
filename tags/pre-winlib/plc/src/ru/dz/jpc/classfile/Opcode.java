//  Opcode.java -- definitions of Java bytecode operations

package ru.dz.jpc.classfile;

import java.io.*;


public class Opcode {


	/*
	 * instance variables
	 */

	/**
	 * The exact bytecode for this instruction
	 */
	public int code;           
	/**
	 * instruction length (0 if variable)
	 */
    public int length;		
    /**
     * kind (classification) of operation
     */
    public int kind;		
    /**
     * flag bits
     */
    public int flags;		
    /**
     * variant for distinguishing similar ops
     */
    public int var;		
    /**
     * number of words popped
     */
    public int pop;		// 
    public String push;		// number and types of words pushed
    /**
     * mnemonic name
     */
    public String name;		 
    /**
     * operator for use in generated code
     */
    public String opr;		 



// global opcode table

static Opcode table[];	



// instruction flags

public static final int I8   = 0x0001;	// instr has 8-bit signed arg
public static final int I16  = 0x0002;	// instr has 16-bit signed arg
public static final int I32  = 0x0004;	// instr has 16-bit signed arg
public static final int U8   = 0x0010;	// instr has 8-bit unsigned arg
public static final int U16  = 0x0020;	// instr has 16-bit unsigned arg
public static final int MORE = 0x0080;	// instr has further additional args

public static final int NFT  = 0x0100;	// no fallthrough to following instr
public static final int PC   = 0x0200;	// argument is PC relative
public static final int CTAB = 0x0800;	// argument indexes constant table

public static final int UNS  = 0x1000;	// unsigned variant of a shift
public static final int SWCH = 0x2000;	// switch instruction
public static final int INST = 0x4000;	// instanceof (including cast & aastore)
public static final int JSRI = 0x8000;	// JSR instruction flag



// newarray types

public static final int T_BOOLEAN	=  4;
public static final int T_CHAR		=  5;
public static final int T_FLOAT		=  6;
public static final int T_DOUBLE	=  7;
public static final int T_BYTE		=  8;
public static final int T_SHORT		=  9;
public static final int T_INT		= 10;
public static final int T_LONG		= 11;



// symbolic definitions of operations and classes of operations
// (these generally don't have to match Java bytecode codes, although many do)
// Constants are defined for wide classes of similar instructions, and 
// specific instances of those instructions are listed under the class.
// The translator can deal with simliar instructions all at once, while
// the interpreter deals with each individual instruction.


public static final int NOP  =  0; // Class of opcodes that do nothing in the 
public static final int POP  = 87; // translator, and their specific numbers
public static final int POP2 = 88;

public static final int CONST       =  1; // constant load class of ops
public static final int ACONST_NULL =  1; // and specific bytecode of the class
public static final int ICONST_M1   =  2;
public static final int ICONST_0    =  3;
public static final int ICONST_1    =  4;
public static final int ICONST_2    =  5;
public static final int ICONST_3    =  6;
public static final int ICONST_4    =  7;
public static final int ICONST_5    =  8;
public static final int LCONST_0    =  9;
public static final int LCONST_1    = 10;
public static final int FCONST_0    = 11;
public static final int FCONST_1    = 12;
public static final int FCONST_2    = 13;
public static final int DCONST_0    = 14;
public static final int DCONST_1    = 15;
public static final int BIPUSH      = 16;
public static final int SIPUSH      = 17;

public static final int LDC         = 18;	// ldc, ldc2, ldc2_w
public static final int LDC_W       = 19;      
public static final int LDC2_W      = 20;

public static final int LOAD        = 21;	// all non-array loads
public static final int ILOAD       = 21;      // and specific instances
public static final int LLOAD       = 22;
public static final int FLOAD       = 23;
public static final int DLOAD       = 24;
public static final int ALOAD       = 25;
public static final int ILOAD_0     = 26;
public static final int ILOAD_1     = 27;
public static final int ILOAD_2     = 28;
public static final int ILOAD_3     = 29;
public static final int LLOAD_0     = 30;
public static final int LLOAD_1     = 31;
public static final int LLOAD_2     = 32;
public static final int LLOAD_3     = 33;
public static final int FLOAD_0     = 34;
public static final int FLOAD_1     = 35;
public static final int FLOAD_2     = 36;
public static final int FLOAD_3     = 37;
public static final int DLOAD_0     = 38;
public static final int DLOAD_1     = 39;
public static final int DLOAD_2     = 40;
public static final int DLOAD_3     = 41;
public static final int ALOAD_0     = 42;
public static final int ALOAD_1     = 43;
public static final int ALOAD_2     = 44;
public static final int ALOAD_3     = 45;

public static final int STORE        = 54;	// all simple store operations
public static final int ISTORE       = 54;      // and specific instances
public static final int LSTORE       = 55;
public static final int FSTORE       = 56;
public static final int DSTORE       = 57;
public static final int ASTORE       = 58;
public static final int ISTORE_0     = 59;
public static final int ISTORE_1     = 60;
public static final int ISTORE_2     = 61;
public static final int ISTORE_3     = 62;
public static final int LSTORE_0     = 63;
public static final int LSTORE_1     = 64;
public static final int LSTORE_2     = 65;
public static final int LSTORE_3     = 66;
public static final int FSTORE_0     = 67;
public static final int FSTORE_1     = 68;
public static final int FSTORE_2     = 69;
public static final int FSTORE_3     = 70;
public static final int DSTORE_0     = 71;
public static final int DSTORE_1     = 72;
public static final int DSTORE_2     = 73;
public static final int DSTORE_3     = 74;
public static final int ASTORE_0     = 75;
public static final int ASTORE_1     = 76;
public static final int ASTORE_2     = 77;
public static final int ASTORE_3     = 78;

public static final int ARRAYLOAD   = 46;	// array load operations
public static final int IALOAD      = 46;      // and specific instances
public static final int LALOAD      = 47;
public static final int FALOAD      = 48;
public static final int DALOAD      = 49;
public static final int AALOAD      = 50;
public static final int BALOAD      = 51;
public static final int CALOAD      = 52;
public static final int SALOAD      = 53;

public static final int ARRAYSTORE  = 79;	// array store operation
public static final int IASTOR      = 79;      // and specific instances
public static final int LASTOR      = 80;
public static final int FASTOR      = 81;
public static final int DASTOR      = 82;
public static final int AASTOR      = 83;
public static final int BASTOR      = 84;
public static final int CASTOR      = 85;
public static final int SASTOR      = 86;

public static final int DUP   = 89;	// dup
public static final int DUPX1 = 90;	// dup_x1
public static final int DUPX2 = 91;	// dup_x2
public static final int DUP2  = 92;	// dup2
public static final int D2X1  = 93;	// dup2_x1
public static final int D2X2  = 94;	// dup2_x2
public static final int SWAP  = 95;	// swap

public static final int BINOP = 96;	// binary operators 
public static final int IADD  = 96;    // and their specific instances
public static final int LADD  = 97;
public static final int FADD  = 98;
public static final int DADD  = 99;
public static final int ISUB  =100;
public static final int LSUB  =101;
public static final int FSUB  =102;
public static final int DSUB  =103;
public static final int IMUL  =104;
public static final int LMUL  =105;
public static final int FMUL  =106;
public static final int DMUL  =107;
public static final int FDIV  =110;
public static final int DDIV  =111;
public static final int IAND  = 126;
public static final int LAND  = 127;
public static final int IOR   = 128;
public static final int LOR   = 129;
public static final int IXOR  = 130;
public static final int LXOR  = 131;

public static final int IINC = 132;	// iinc (all by itself)

public static final int DIVOP = 108;	// division ops that throw division by
public static final int IDIV  = 108;    // zero exceptions
public static final int LDIV  = 109;   
public static final int IREM  = 112;   
public static final int LREM  = 113;   

public static final int FREM = 114;	// float (or double) remainder ops
public static final int DREM = 115;	// and the specific double variant

public static final int SHIFT = 120;	// shift operators
public static final int ISHL =  120;   // and the specific instances
public static final int LSHL =  121;
public static final int ISHR =  122;
public static final int LSHR =  123;
public static final int IUSHR = 124;
public static final int LUSHR = 125;

public static final int UNOP =    116;	// unary operators 
public static final int INEG =    116; // and the specific instances of that class
public static final int LNEG =	   117;
public static final int FNEG =	   118;
public static final int DNEG =	   119;
public static final int I2L =	   133;
public static final int I2F =	   134;
public static final int I2D =	   135;
public static final int L2I =	   136;
public static final int L2F =	   137;
public static final int L2D =	   138;
public static final int F2D =	   141;
public static final int D2F =	   144;
public static final int INT2BYTE =   145;
public static final int INT2CHAR =   146;
public static final int INT2SHORT =  147;

public static final int FTOI =  139;	// f2l, d2l, f2i, d2i conversions
public static final int F2I  =  139;   // and specific instances
public static final int F2L  =  140;
public static final int D2I  =  142;
public static final int D2L  =  143;

public static final int CMP   = 148;	// compares without branching
public static final int LCMP  = 148;
public static final int FCMPL = 149;
public static final int FCMPG = 150;
public static final int DCMPL = 151;
public static final int DCMPG = 152;

public static final int IFZRO =  153;	// compare to zero and branches
public static final int IFEQ   = 153;
public static final int IFNE   = 154;
public static final int IFLT   = 155;
public static final int IFGE   = 156;
public static final int IFGT   = 157;
public static final int IFLE   = 158;
public static final int IFNULL = 198;
public static final int IFNONNULL = 199;

public static final int IFCMP = 159;	// compare and branches
public static final int IF_ICMPEQ = 159;
public static final int IF_ICMPNE = 160;
public static final int IF_ICMPLT = 161;
public static final int IF_ICMPGE = 162;
public static final int IF_ICMPGT = 163;
public static final int IF_ICMPLE = 164;
public static final int IF_ACMPEQ = 165;
public static final int IF_ACMPNE = 166;

public static final int GOTO   = 167;	// goto, goto_w
public static final int GOTO_W = 200;

public static final int JSR   = 168;	// jsr, jsr_w
public static final int JSR_W = 201;

public static final int RET   = 169;	// ret, ret_w
public static final int TBLSW = 170;	// tableswitch
public static final int LKPSW = 171;	// lookupswitch

public static final int RETV    = 172;	// return value (iret, aret, etc.)
public static final int IRETURN = 172;
public static final int LRETURN = 173;
public static final int FRETURN = 174;
public static final int DRETURN = 175;
public static final int ARETURN = 176;

public static final int RETRN   = 177;	// return, no value

public static final int GETS = 178;	// getstatic
public static final int PUTS = 179;	// putstatic
public static final int GETF = 180;	// getfield
public static final int PUTF = 181;	// putfield

public static final int IVIRT = 182;	// invokevirtual
public static final int INONV = 183;	// invokenonvirtual
public static final int ISTAT = 184;	// invokestatic
public static final int IINTR = 185;	// invokeinterface

public static final int NEW = 187;	// new
public static final int NEWA = 188;	// newarray
public static final int ANEWA = 189;	// anewarray
public static final int ALEN = 190;	// arraylength
public static final int THROW = 191;	// athrow
public static final int ACAST = 192;	// checkcast
public static final int INSTC = 193;	// instanceof
public static final int MENTR = 194;	// monitorenter
public static final int MEXIT = 195;	// monitorexit
public static final int MNEWA = 197;	// multianewarray
public static final int BPOINT = 202;  // breakpoint

public static final int WIDE = 196;	// wide (special instruction form)


//  opcode table initialization

static {

    table = new Opcode[256];

    //  op len kind  pop push   name               var flags

    op(ACONST_NULL, 1, CONST, 0, "a",   "aconst_null",      0, 0);
    op(ICONST_M1,   1, CONST, 0, "i",   "iconst_m1",       -1, 0);
    op(ICONST_0,    1, CONST, 0, "i",   "iconst_0",         0, 0);
    op(ICONST_1,    1, CONST, 0, "i",   "iconst_1",         1, 0);
    op(ICONST_2,    1, CONST, 0, "i",   "iconst_2",         2, 0);
    op(ICONST_3,    1, CONST, 0, "i",   "iconst_3",         3, 0);
    op(ICONST_4,    1, CONST, 0, "i",   "iconst_4",         4, 0);
    op(ICONST_5,    1, CONST, 0, "i",   "iconst_5",         5, 0);
    op(LCONST_0,    1, CONST, 0, "xl",  "lconst_0",         0, 0);
    op(LCONST_1,    1, CONST, 0, "xl",  "lconst_1",         1, 0);
    op(FCONST_0,    1, CONST, 0, "f",   "fconst_0",         0, 0);
    op(FCONST_1,    1, CONST, 0, "f",   "fconst_1",         1, 0);
    op(FCONST_2,    1, CONST, 0, "f",   "fconst_2",         2, 0);
    op(DCONST_0,    1, CONST, 0, "xd",  "dconst_0",         0, 0);
    op(DCONST_1,    1, CONST, 0, "xd",  "dconst_1",         1, 0);
    op(BIPUSH,      2, CONST, 0, "i",   "bipush",           0, I8);
    op(SIPUSH,      3, CONST, 0, "i",   "sipush",           0, I16);
    op(LDC,         2, LDC,   0, "*",   "ldc",              0, CTAB + U8);
    op(LDC_W,       3, LDC,   0, "*",   "ldc_w",            0, CTAB + U16);
    op(LDC2_W,      3, LDC,   0, "x*",  "ldc2_w",           0, CTAB + U16);

    op(ILOAD,   2, LOAD,  0, "i",   "iload",            0, U8);
    op(ILOAD_0, 1, LOAD,  0, "i",   "iload_0",          0, 0);
    op(ILOAD_1, 1, LOAD,  0, "i",   "iload_1",          1, 0);
    op(ILOAD_2, 1, LOAD,  0, "i",   "iload_2",          2, 0);
    op(ILOAD_3, 1, LOAD,  0, "i",   "iload_3",          3, 0);
    op(LLOAD,   2, LOAD,  0, "xl",  "lload",            0, U8);
    op(LLOAD_0, 1, LOAD,  0, "xl",  "lload_0",          0, 0);
    op(LLOAD_1, 1, LOAD,  0, "xl",  "lload_1",          1, 0);
    op(LLOAD_2, 1, LOAD,  0, "xl",  "lload_2",          2, 0);
    op(LLOAD_3, 1, LOAD,  0, "xl",  "lload_3",          3, 0);
    op(FLOAD,   2, LOAD,  0, "f",   "fload",            0, U8);
    op(FLOAD_0, 1, LOAD,  0, "f",   "fload_0",          0, 0);
    op(FLOAD_1, 1, LOAD,  0, "f",   "fload_1",          1, 0);
    op(FLOAD_2, 1, LOAD,  0, "f",   "fload_2",          2, 0);
    op(FLOAD_3, 1, LOAD,  0, "f",   "fload_3",          3, 0);
    op(DLOAD,   2, LOAD,  0, "xd",  "dload",            0, U8);
    op(DLOAD_0, 1, LOAD,  0, "xd",  "dload_0",          0, 0);
    op(DLOAD_1, 1, LOAD,  0, "xd",  "dload_1",          1, 0);
    op(DLOAD_2, 1, LOAD,  0, "xd",  "dload_2",          2, 0);
    op(DLOAD_3, 1, LOAD,  0, "xd",  "dload_3",          3, 0);
    op(ALOAD,   2, LOAD,  0, "a",   "aload",            0, U8);
    op(ALOAD_0, 1, LOAD,  0, "a",   "aload_0",          0, 0);
    op(ALOAD_1, 1, LOAD,  0, "a",   "aload_1",          1, 0);
    op(ALOAD_2, 1, LOAD,  0, "a",   "aload_2",          2, 0);
    op(ALOAD_3, 1, LOAD,  0, "a",   "aload_3",          3, 0);

    op(ISTORE,   2, STORE, 1, "",    "istore",           0, U8);
    op(ISTORE_0, 1, STORE, 1, "",    "istore_0",         0, 0);
    op(ISTORE_1, 1, STORE, 1, "",    "istore_1",         1, 0);
    op(ISTORE_2, 1, STORE, 1, "",    "istore_2",         2, 0);
    op(ISTORE_3, 1, STORE, 1, "",    "istore_3",         3, 0);
    op(LSTORE,   2, STORE, 2, "",    "lstore",           0, U8);
    op(LSTORE_0, 1, STORE, 2, "",    "lstore_0",         0, 0);
    op(LSTORE_1, 1, STORE, 2, "",    "lstore_1",         1, 0);
    op(LSTORE_2, 1, STORE, 2, "",    "lstore_2",         2, 0);
    op(LSTORE_3, 1, STORE, 2, "",    "lstore_3",         3, 0);
    op(FSTORE,   2, STORE, 1, "",    "fstore",           0, U8);
    op(FSTORE_0, 1, STORE, 1, "",    "fstore_0",         0, 0);
    op(FSTORE_1, 1, STORE, 1, "",    "fstore_1",         1, 0);
    op(FSTORE_2, 1, STORE, 1, "",    "fstore_2",         2, 0);
    op(FSTORE_3, 1, STORE, 1, "",    "fstore_3",         3, 0);
    op(DSTORE,   2, STORE, 2, "",    "dstore",           0, U8);
    op(DSTORE_0, 1, STORE, 2, "",    "dstore_0",         0, 0);
    op(DSTORE_1, 1, STORE, 2, "",    "dstore_1",         1, 0);
    op(DSTORE_2, 1, STORE, 2, "",    "dstore_2",         2, 0);
    op(DSTORE_3, 1, STORE, 2, "",    "dstore_3",         3, 0);
    op(ASTORE,   2, STORE, 1, "",    "astore",           0, U8);
    op(ASTORE_0, 1, STORE, 1, "",    "astore_0",         0, 0);
    op(ASTORE_1, 1, STORE, 1, "",    "astore_1",         1, 0);
    op(ASTORE_2, 1, STORE, 1, "",    "astore_2",         2, 0);
    op(ASTORE_3, 1, STORE, 1, "",    "astore_3",         3, 0);

    op(IINC, 3, IINC,  0, "",    "iinc",             0, U8 + MORE);
    op(GETS, 3, GETS,  0, "*",   "getstatic",        0, CTAB + U16);
    op(PUTS, 3, PUTS,  1, "",    "putstatic",        0, CTAB + U16);
    op(GETF, 3, GETF,  1, "*",   "getfield",         0, CTAB + U16);
    op(PUTF, 3, PUTF,  2, "",    "putfield",         0, CTAB + U16);

    op(NEW,   3, NEW,   0, "a",   "new",              0, CTAB + U16);
    op(ACAST, 3, ACAST, 0, "",    "checkcast",        0, INST + CTAB + U16);
    op(INSTC, 3, INSTC, 1, "i",   "instanceof",       0, INST + CTAB + U16);

    op(NEWA,   2, NEWA,       1, "a",  "newarray",       0, U8);
    op(ANEWA,  3, ANEWA,      1, "a",  "anewarray",      0, CTAB + U16);
    op(MNEWA,  4, MNEWA,      0, "a",  "multianewarray", 0, CTAB + U16 +MORE);
    op(ALEN,   1, ALEN,       1, "i",  "arraylength",    0, 0);
    op(IALOAD, 1, ARRAYLOAD,  2, "i",  "iaload",         0, 0);
    op(LALOAD, 1, ARRAYLOAD,  2, "xl", "laload",         0, 0);
    op(FALOAD, 1, ARRAYLOAD,  2, "f",  "faload",         0, 0);
    op(DALOAD, 1, ARRAYLOAD,  2, "xd", "daload",         0, 0);
    op(AALOAD, 1, ARRAYLOAD,  2, "a",  "aaload",         0, 0);
    op(BALOAD, 1, ARRAYLOAD,  2, "i",  "baload",         0, 0);
    op(CALOAD, 1, ARRAYLOAD,  2, "i",  "caload",         0, 0);
    op(SALOAD, 1, ARRAYLOAD,  2, "i",  "saload",         0, 0);
    op(IASTOR, 1, ARRAYSTORE, 3, "",   "iastore",        0, 0);
    op(LASTOR, 1, ARRAYSTORE, 4, "",   "lastore",        0, 0);
    op(FASTOR, 1, ARRAYSTORE, 3, "",   "fastore",        0, 0);
    op(DASTOR, 1, ARRAYSTORE, 4, "",   "dastore",        0, 0);
    op(AASTOR, 1, ARRAYSTORE, 3, "",   "aastore",        0, INST);
    op(BASTOR, 1, ARRAYSTORE, 3, "",   "bastore",        0, 0);
    op(CASTOR, 1, ARRAYSTORE, 3, "",   "castore",        0, 0);
    op(SASTOR, 1, ARRAYSTORE, 3, "",   "sastore",        0, 0);

    op(NOP,    1, NOP,   0, "",    "nop",              0, 0);
    op(POP,    1, NOP,   1, "",    "pop",              0, 0);
    op(POP2,   1, NOP,   2, "",    "pop2",             0, 0);
    op(DUP,    1, DUP,   0, "*",   "dup",              0, 0);
    op(DUPX1,  1, DUPX1, 0, "*",   "dup_x1",           0, 0);
    op(DUPX2,  1, DUPX2, 0, "*",   "dup_x2",           0, 0);
    op(DUP2,   1, DUP2,  0, "**",  "dup2",             0, 0);
    op(D2X1,   1, D2X1,  0, "**",  "dup2_x1",          0, 0);
    op(D2X2,   1, D2X2,  0, "**",  "dup2_x2",          0, 0);
    op(SWAP,   1, SWAP,  0, "**",  "swap",             0, 0);

    op(INEG, 	  1, UNOP,  1, "i",   "ineg",             "-", 0);
    op(LNEG, 	  1, UNOP,  2, "xl",  "lneg",             "-", 0);
    op(FNEG, 	  1, UNOP,  1, "f",   "fneg",             "-", 0);
    op(DNEG, 	  1, UNOP,  2, "xd",  "dneg",             "-", 0);
    op(I2L,  	  1, UNOP,  1, "xl",  "i2l",              "", 0);
    op(I2F,  	  1, UNOP,  1, "f",   "i2f",              "", 0);
    op(I2D,  	  1, UNOP,  1, "xd",  "i2d",              "", 0);
    op(L2I,  	  1, UNOP,  2, "i",   "l2i",              "", 0);
    op(L2F,  	  1, UNOP,  2, "f",   "l2f",              "", 0);
    op(L2D,  	  1, UNOP,  2, "xd",  "l2d",              "", 0);
    op(F2I, 	  1, FTOI,  1, "i",   "f2i",              "dtoi", 0);
    op(F2L, 	  1, FTOI,  1, "xl",  "f2l",              "dtol", 0);
    op(F2D, 	  1, UNOP,  1, "xd",  "f2d",              "", 0);
    op(D2I, 	  1, FTOI,  2, "i",   "d2i",              "dtoi", 0);
    op(D2L, 	  1, FTOI,  2, "xl",  "d2l",              "dtol", 0);
    op(D2F, 	  1, UNOP,  2, "f",   "d2f",              "", 0);
    op(INT2BYTE,  1, UNOP,  1, "i",   "int2byte",         "(Byte)", 0);
    op(INT2CHAR,  1, UNOP,  1, "i",   "int2char",         "(Char)", 0);
    op(INT2SHORT, 1, UNOP,  1, "i",   "int2short",        "(Short)", 0);

    op(IADD,  1, BINOP, 2, "i",   "iadd",             " + ", 0);
    op(LADD,  1, BINOP, 4, "xl",  "ladd",             " + ", 0);
    op(FADD,  1, BINOP, 2, "f",   "fadd",             " + ", 0);
    op(DADD,  1, BINOP, 4, "xd",  "dadd",             " + ", 0);
    op(ISUB,  1, BINOP, 2, "i",   "isub",             " - ", 0);
    op(LSUB,  1, BINOP, 4, "xl",  "lsub",             " - ", 0);
    op(FSUB,  1, BINOP, 2, "f",   "fsub",             " - ", 0);
    op(DSUB,  1, BINOP, 4, "xd",  "dsub",             " - ", 0);
    op(IMUL,  1, BINOP, 2, "i",   "imul",             " * ", 0);
    op(LMUL,  1, BINOP, 4, "xl",  "lmul",             " * ", 0);
    op(FMUL,  1, BINOP, 2, "f",   "fmul",             " * ", 0);
    op(DMUL,  1, BINOP, 4, "xd",  "dmul",             " * ", 0);
    op(IDIV,  1, DIVOP, 2, "i",   "idiv",             " / ", 0);
    op(LDIV,  1, DIVOP, 4, "xl",  "ldiv",             " / ", 0);
    op(FDIV,  1, BINOP, 2, "f",   "fdiv",             " / ", 0);
    op(DDIV,  1, BINOP, 4, "xd",  "ddiv",             " / ", 0);
    op(IREM,  1, DIVOP, 2, "i",   "irem",             " % ", 0);
    op(LREM,  1, DIVOP, 4, "xl",  "lrem",             " % ", 0);
    op(FREM,  1, FREM,  2, "f",   "frem",             " % ", 0);
    op(DREM,  1, FREM,  4, "xd",  "drem",             " % ", 0);
    op(IAND,  1, BINOP, 2, "i",   "iand",             " & ", 0);
    op(LAND,  1, BINOP, 4, "xl",  "land",             " & ", 0);
    op(IOR,   1, BINOP, 2, "i",   "ior",              " | ", 0);
    op(LOR,   1, BINOP, 4, "xl",  "lor",              " | ", 0);
    op(IXOR,  1, BINOP, 2, "i",   "ixor",             " ^ ", 0);
    op(LXOR,  1, BINOP, 4, "xl",  "lxor",             " ^ ", 0);
    op(ISHL,  1, SHIFT, 2, "i",   "ishl",             " << ", 0);
    op(ISHR,  1, SHIFT, 2, "i",   "ishr",             " >> ", 0);
    op(IUSHR, 1, SHIFT, 2, "i",   "iushr",            " >> ", UNS);
    op(LSHL,  1, SHIFT, 3, "xl",  "lshl",             " << ", 0);
    op(LSHR,  1, SHIFT, 3, "xl",  "lshr",             " >> ", 0);
    op(LUSHR, 1, SHIFT, 3, "xl",  "lushr",            " >> ", UNS);

    op(LCMP,  	  1, CMP,   4, "i",   "lcmp",             0, 0);
    op(FCMPL, 	  1, CMP,   2, "i",   "fcmpl",            -1, 0);
    op(FCMPG, 	  1, CMP,   2, "i",   "fcmpg",            +1, 0);
    op(DCMPL, 	  1, CMP,   4, "i",   "dcmpl",            -1, 0);
    op(DCMPG, 	  1, CMP,   4, "i",   "dcmpg",            +1, 0);
    op(IFNULL,    3, IFZRO, 1, "",    "ifnull",           " == ", PC + I16);
    op(IFEQ, 	  3, IFZRO, 1, "",    "ifeq",             " == ", PC + I16);
    op(IFLT, 	  3, IFZRO, 1, "",    "iflt",             " < ",  PC + I16);
    op(IFLE, 	  3, IFZRO, 1, "",    "ifle",             " <= ", PC + I16);
    op(IFNE, 	  3, IFZRO, 1, "",    "ifne",             " != ", PC + I16);
    op(IFNONNULL, 3, IFZRO, 1, "",    "ifnonnull",        " != ", PC + I16);
    op(IFGT,      3, IFZRO, 1, "",    "ifgt",             " > ",  PC + I16);
    op(IFGE,      3, IFZRO, 1, "",    "ifge",             " >= ", PC + I16);
    op(IF_ICMPEQ, 3, IFCMP, 2, "",    "if_icmpeq",        " == ", PC + I16);
    op(IF_ICMPNE, 3, IFCMP, 2, "",    "if_icmpne",        " != ", PC + I16);
    op(IF_ICMPLT, 3, IFCMP, 2, "",    "if_icmplt",        " < ",  PC + I16);
    op(IF_ICMPGT, 3, IFCMP, 2, "",    "if_icmpgt",        " > ",  PC + I16);
    op(IF_ICMPLE, 3, IFCMP, 2, "",    "if_icmple",        " <= ", PC + I16);
    op(IF_ICMPGE, 3, IFCMP, 2, "",    "if_icmpge",        " >= ", PC + I16);
    op(IF_ACMPEQ, 3, IFCMP, 2, "",    "if_acmpeq",        " == ", PC + I16);
    op(IF_ACMPNE, 3, IFCMP, 2, "",    "if_acmpne",        " != ", PC + I16);

    op(TBLSW,  0, TBLSW, 1, "",    "tableswitch",      0, NFT + SWCH + MORE);
    op(LKPSW,  0, LKPSW, 1, "",    "lookupswitch",     0, NFT + SWCH + MORE);
    op(GOTO,   3, GOTO,  0, "",    "goto",             0, NFT + PC + I16);
    op(GOTO_W, 5, GOTO,  0, "",    "goto_w",           0, NFT + PC + I32);
    op(JSR,    3, JSR,   0, "",    "jsr",              0, JSRI + PC + I16);
    op(JSR_W,  5, JSR,   0, "",    "jsr_w",            0, JSRI + PC + I32);
    op(RET,    2, RET,   0, "",    "ret",              0, NFT + U8);

    op(IVIRT, 	3, IVIRT, 1, "",    "invokevirtual",    0, CTAB + U16);
    op(INONV, 	3, INONV, 1, "",    "invokenonvirtual", 0, CTAB + U16);
    op(ISTAT, 	3, ISTAT, 0, "",    "invokestatic",     0, CTAB + U16);
    op(IINTR, 	5, IINTR, 1, "",    "invokeinterface",  0, CTAB + U16 + MORE);
    op(IRETURN, 1, RETV,  0, "",    "ireturn",          0, NFT);
    op(LRETURN, 1, RETV,  0, "",    "lreturn",          0, NFT);
    op(FRETURN, 1, RETV,  0, "",    "freturn",          0, NFT);
    op(DRETURN, 1, RETV,  0, "",    "dreturn",          0, NFT);
    op(ARETURN, 1, RETV,  0, "",    "areturn",          0, NFT);
    op(RETRN,   1, RETRN, 0, "",    "return",           0, NFT);

    op(THROW, 1, THROW, 1, "",    "athrow",           0, NFT);
    op(BPOINT,1, NOP,   0, "",    "breakpoint",       0, 0);
    op(MENTR, 1, MENTR, 1, "",    "monitorenter",     0, 0);
    op(MEXIT, 1, MEXIT, 1, "",    "monitorexit",      0, 0);

    op(WIDE, 0, WIDE,  0, "",    "wide",	            0, 0);
}


//  op(code,length,kind,pop,push,name,var,flags) -- initialize opcode entry

static private void op (int code, int length, int kind, int pop, String push,
    String name, int var, int flags)
{
    Opcode o = new Opcode();
    table[code] = o;

    o.code = code;
    o.length = length;
    o.kind = kind;
    o.pop = pop;
    o.push = push;
    o.name = name;
    o.var = var;
    o.flags = flags;
}


//  op(code,length,kind,pop,push,name,opr,flags) -- initialize special entry
//
//  Used with a String "opr" in place of the "var" parameter.

static private void op (int code, int length, int kind, int pop, String push,
    String name, String opr, int flags)
{
    op(code, length, kind, pop, push, name, 0, flags);
    table[code].opr = opr;
}


public static Opcode
lookupOpcode (int opc)
{
    return table [opc];
}

public synchronized final String
toString ()
{
    return name;
}

} // class Opcode
