package ru.dz.phantom.mkbulk;

import java.io.*;

/**
 * The Phantom bulk maker.
 * Creates 'classes' file.
 */


/**
 * @converter_from_C_to_Java: oleg.kakaulin
 *
 */
class Main {

	final static int max_classes_in_bulk = 100;
	final static int classname_offset = 15;
	final static int header_size = 512;
	static String[] cl_arguments = new String[max_classes_in_bulk];

	/**
	 * @param args
	 */
	public static void main(String[] args) throws IOException {
		// TODO Auto-generated method stub
		System.out.println("mkbulk: combine Phantom class files to a special");
		System.out.println("bulk file to bundle with kernel (classes boot module)");
		if (args.length < 2) {
			System.out.println("\nUsage: mkbulk outfile infile [...]");
			System.out.println("\n       OR");
			System.out.println("\n       mkbulk -l listfile");
			return;
		}

		System.out.println("processing...");
		
		// Prepare command line arguments
		if (args[0].equals("-l")) {
			// Open list file
			BufferedReader list = new BufferedReader(new FileReader(args[1]));
			String s = new String();
			int k = 0;
			while ((s = list.readLine()) != null) {
				cl_arguments[k] = s;
				k++;
			}
			list.close();
		}
		else {
			cl_arguments = args;
		}

		// Create the output file
		DataOutputStream out = new DataOutputStream(
				new BufferedOutputStream(new FileOutputStream(cl_arguments[0])));

		// Scan and collect input files
		for (int i = 1; (i < cl_arguments.length) && (cl_arguments[i] != null); i++) 
		{
			// Test .pc extension of a filename
			String fn = cl_arguments[i];
			
			// Warning! Cygwin hack! /cygdrive/c/aaa -> c:/aaa
			
			//System.out.println("Process <" + fn + ">");
			
			if( (fn.length() > 11) && fn.substring(0,10).equals("/cygdrive/") )
				fn = fn.substring(10, 11) + ":" + fn.substring(11); 
			
			System.out.println("Process <" + fn + ">");
			
			if (IsNotPCExtensionOf(fn)) continue;
			// Open next input file
			DataInputStream in = new DataInputStream(
					new BufferedInputStream(new FileInputStream(fn)));
			// Calculate file length
			int av = in.available();
			// System.out.println("mkbulk: length of <" + cl_arguments[i] + "> = " + av);
			// Load the file into buffer
			byte[] buffer = new byte[av]; 
			int n = in.read(buffer);
			// System.out.println("mkbulk: " + n + " bytes was read");
			in.close();
			
			// Test Phantom class file signature
			if (PCFileSignatureIsWrong(buffer, fn)) continue;
			
			// Prepare and write header of the class
			writeHeader(out, buffer, av);
			
			// Write Phantom class to the bulk
			out.write(buffer);
		}
		
		out.flush();
		out.close();
		System.out.println("Ok. The bulk file <" + cl_arguments[0] + "> created.");
	}
	
	static void writeHeader(DataOutputStream os, byte[] buf, int len) throws IOException {
		try {
			byte[] header = new byte[header_size];
			byte[] length = new byte[4];
			
			// Prepare the header
			int k = 0;
			for (int i = classname_offset; i < len; i++) {
				if (buf[i] <= ' ') {
					header[k] = 0;
					break;
				}
				else {
					header[k] = buf[i];
					k++;
				}
			}
			// Prepare the length
			length[3] = (byte) (len >>> 24);
			length[2] = (byte) (len >>> 16);
			length[1] = (byte) (len >>> 8);
			length[0] = (byte) len;
			// Write header to the bulk
			os.write(header);
			os.write(length);
		} catch (IOException e) {
			System.err.println("mkbulk: Can't write the header to the bulk file.");
		}
	}

	static boolean IsNotPCExtensionOf(String fn) {
		String ext;
		if (fn.length() > 2)
			ext = fn.substring(fn.length()- 3);
		else 
			ext = "";
		if (ext.equalsIgnoreCase(".PC")) 
			return false;
		else {
			System.err.println("mkbulk: Input file <" + fn + "> has no .pc extension. Skip.");
			return true;
		}
	}
	
	static boolean PCFileSignatureIsWrong(byte[] buf, String fn) {
		boolean fl = false;
		
//		System.out.println(buf[0]);
		if (buf[0] != 'p') fl = true;
//		System.out.println(buf[1]);
		if (buf[1] != 'h') fl = true;
//		System.out.println(buf[2]);
		if (buf[2] != 'f') fl = true;
//		System.out.println(buf[3]);
		if (buf[3] != 'r') fl = true;
//		System.out.println(buf[4]);
		if (buf[4] != ':') fl = true;
//		System.out.println(buf[5]);
		if (buf[5] != 'C') fl = true;
		
		if (fl) System.err.println("mkbulk: File <" + fn + "> is not Phantom class file. Skip.");
		return fl;
	}

}
