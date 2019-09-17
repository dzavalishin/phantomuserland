package ru.dz.plc;

import java.util.LinkedList;
import java.io.IOException;
import ru.dz.plc.compiler.Grammar;
import ru.dz.plc.parser.*;
import ru.dz.plc.util.*;

import java.io.*;

/**
 * <p>Title: Phantom Language Compiler main</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class PlcMain {
	static LinkedList<Token> tokens = new LinkedList<Token>();
	private static LinkedList<File> classFileSearchPath = new LinkedList<File>();
	private static String outputPath = null;
	
	public PlcMain() { }

	public static void main(String[] args) {
		//System.out.println( Version.NAME + " " +Version.VERSION+ ", build from "+CompileDate.getDate());

		if(args.length < 1)
		{
			System.out.println(
					"plc [flags] file...\n\n"+
					"Compile .ph files to .pc class files.\n\n"+
					"Flags:\n"+
					"\t-Ipath\t- path to the directory to look for .pc class files of imported classes\n"+
					"\t-opath\t- path to put created .pc class files to\n"+
					""
					);
			return;
		}
		
		try { go(args); }
		catch( PlcException e ) {
			System.out.println("Failed: " + e.toString());
		}
		catch( java.io.FileNotFoundException e) {
			System.out.println("File not found: " + e.toString());
		}
		catch( java.io.IOException e) {
			System.out.println("IO error: " + e.toString());
		}

	}

	static void go(String[] args) throws FileNotFoundException,
	IOException, FileNotFoundException, PlcException
	{
		for (int i = 0; i < args.length; i++) {
			String arg = args[i];
			
			if(arg.charAt(0) == '-')
			{
				processFlag(arg);
				continue;
			}
			
			//System.out.println("Compiling " + arg);

			compile(arg);
		}
	}


	private static void processFlag(String arg) {
		if(arg.length() < 2)
		{
			System.err.println("Won't compile stdin");
			return;
		}
		String argValue = arg.substring(2);
		switch(arg.charAt(1))
		{
		case 'I':
			classFileSearchPath.add( new File(argValue) );
			return;
		
		case 'o':
			outputPath = argValue;
			return;
			
		default:
			System.err.println("Unknown flag: "+arg);
			return;
		}
		
	}

	public static LinkedList<File> getClassFileSearchPath() {		return classFileSearchPath;	}
	public static String getOutputPath() {		return outputPath;	}

	
	
	static void compile( String fn ) throws FileNotFoundException, PlcException,
	IOException {
		FileInputStream  fis = new FileInputStream ( fn );

		Lex l = new Lex(tokens,fn);

		l.set_input(fis);

		Grammar g = new Grammar(l,fn);

		try {
			//Node all = 
			g.parse();

			if(g.get_error_count() == 0) 
				g.codegen();

			if(g.get_warning_count() > 0)
				System.out.println(">> "+g.get_warning_count()+" warnings found");

			if(g.get_error_count() > 0)
			{
				//outf.delete();
				System.out.println(">> "+g.get_error_count()+" errors found");
			}
			//else				System.out.println(">> EOF");
		} catch( PlcException e )
		{
			System.out.println("Compile failed: "+e.toString());
			// TODO in fact we should try to compile as many classes as possible instead 
			System.exit(1);		
		}
	}


}

