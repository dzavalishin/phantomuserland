package ru.dz.phantasm;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.LinkedList;
import java.io.IOException;
import ru.dz.phantom.code.*;
import ru.dz.plc.parser.*;
import ru.dz.plc.util.*;

import java.io.*;

public class phantasmMain {
  static LinkedList<Token> tokens = new LinkedList<Token>();

  public phantasmMain() {}

  public static void main(String[] args) throws FileNotFoundException,
      IOException, FileNotFoundException {
    System.out.println("PhantAsm 0.1");

    try { go(args); }
    catch( PlcException e ) {
      System.out.println("Failed: " + e.toString());

    }

  }

  static void go(String[] args) throws FileNotFoundException,
      IOException, FileNotFoundException, PlcException
  {
    for (int i = 0; i < args.length; i++) {
      System.out.print("Assembling " + args[i]);
      System.out.println();
      assemble(args[i]);
    }
  }


  static void assemble( String fn ) throws FileNotFoundException, PlcException,
      IOException {
    FileInputStream  fis = new FileInputStream ( fn );
    //FileOutputStream fos = new FileOutputStream( fn + ".pc" );

    FileWriter list = new FileWriter(fn + ".lst");
    
    File outf = new File(fn + ".pc");
    outf.delete();
    RandomAccessFile of = new RandomAccessFile( outf, "rw" );
    Codegen cg = new Codegen(of, list);

    Lex l = new Lex(tokens,fn);

    l.set_input(fis);

    grammar g = new grammar(l,fn,cg);

    try {
      g.parse();
      cg.relocate_all();
      //cg.finalizer(); // close file
      of.close();
      cg = null;
      if(g.get_warning_count() > 0)
        System.out.println(">> "+g.get_warning_count()+" warnings found");

      if(g.get_error_count() > 0)
      {
        outf.delete();
        System.out.println(">> "+g.get_error_count()+" errors found");
      }
      else
        System.out.println(">> EOF");
    } catch( PlcException e )
    {
      of.close();
      //cg.finalizer(); // close file
      cg = null;
      outf.delete();
      System.out.println("Assembly failed: "+e.toString());
    }
  }


}



