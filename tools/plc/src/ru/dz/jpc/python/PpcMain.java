package ru.dz.jpc.python;

import java.io.File;
import java.io.IOException;
import java.util.logging.Logger;

import javax.xml.parsers.ParserConfigurationException;

import org.xml.sax.SAXException;

import ru.dz.plc.PlcMain;
import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.util.PlcException;

public class PpcMain {

	static final Logger log = Logger.getLogger("ru.dz.jpc.python");

	
	/**
	 * @param args
	 * @throws ParserConfigurationException 
	 * @throws IOException 
	 * @throws SAXException 
	 * @throws ConnvertException 
	 * @throws PlcException 
	 */
	public static void main(String[] args) throws ParserConfigurationException, SAXException, IOException, ConnvertException, PlcException {
		boolean err = go(args);
		if(err)
			System.exit(1);
	}

	/*
	private static void convert(PythonFrontendXML pfx) {
		
	}*/

	public static boolean go(String[] args) throws ParserConfigurationException, SAXException, IOException, ConnvertException, PlcException {
		ClassMap cm = ClassMap.get_map(); // Create

        PlcMain.addClassFileSearchParh( new File("test/pc") );
        PlcMain.setOutputPath("test/pc");

		
		cm.do_import(".internal.object"); // TODO hack!
		
		
		
		PythonFrontendXML pfx = new PythonFrontendXML();
		
		pfx.load(new File("test/pyfront/testinput.out"));
		
		if(pfx.getErrorCount() > 0)
		{
			log.severe("XML load errors, stopped compiling");
			return true;
		}
		
		//pfx.print();
		
		//PythonConvertor pc = new PythonConvertor();
		
		pfx.convert();

		if(pfx.getErrorCount() > 0)
		{
			log.severe("XML process errors, stopped compiling");
			return true;
		}
		
		cm.codegen();
		
		return false;
	}

	
	
}
