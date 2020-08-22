package ru.dz.plc.compiler;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintStream;
import java.io.RandomAccessFile;

import ru.dz.plc.util.PlcException;

public interface IMethodTable {

	public abstract void set_ordinal(Method m, int ord) throws PlcException;

	public abstract void set_ordinals() throws PlcException;

	public abstract int slots_needed() throws PlcException;

	public abstract void preprocess(ParseState ps) throws PlcException;

//	public abstract void codegen(RandomAccessFile os, FileWriter lst,
//			BufferedWriter llvmFile, CodeGeneratorState s, String version)
//			throws IOException, PlcException;

	/*
	public abstract void codegen(RandomAccessFile os, FileWriter lst,
			BufferedWriter llvmFile, BufferedWriter c_File, 
			CodeGeneratorState s, String version) throws IOException, PlcException;
	*/
	public abstract void codegen(CodeWriters cw, CodeGeneratorState s) throws IOException, PlcException;
	
	//public abstract Method get(String name);

	//public abstract boolean have(String name);

	public abstract Method add(Method m) throws PlcException;

	//public abstract Method add(String name, PhantomType type, boolean constructor ) throws PlcException;

	
	public abstract void print(PrintStream ps) throws PlcException;

	public abstract void dump();


}