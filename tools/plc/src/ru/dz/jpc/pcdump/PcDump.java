package ru.dz.jpc.pcdump;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

import ru.dz.plc.compiler.ClassMap;
import ru.dz.plc.util.PlcException;

/**
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * Date: 04.10.2009 22:50:29
 * @author Irek
 */
public class PcDump {
//	static final Logger log = Logger.getLogger(PcDump.class.getName());

    static String[] paths;

    public static void main(String args[]) throws PlcException, IOException {
        System.out.println("PcDump 0.1");

        options(args);			// process options

        // process all command line classes
        for (int i = 0; i < args.length; i++) {
            if (args[i] != null) {
                try {
                    doDump(args[i]);		// process other arguments
                } catch (PlcException e) {
                    System.out.println("Error: "+e.toString());
                } catch (IOException e) {
                    System.out.println(e.getMessage());
                }
            }
        }
    }

   	private static void options(String args[]) {
        if (args.length == 0) {
            System.out.println(
                    "Deassemble .pc class files."+
                    "    Usage: java PcDump file.pc [file2.pc ...]"
            );
        }
	}


	private static void doDump(String name) throws PlcException, IOException {
        System.out.println("");
        System.out.println("Dump file "+ name);
        RandomAccessFile is = findFile(name);
        if (is == null) throw new PlcException("dump", "File not found", name);

        ClassMap classes = ClassMap.get_map();
        classes.do_import(".internal.object");
        classes.do_import(".internal.int");


        ClassInfoLoader ci = new ClassInfoLoader(is);
        ci.setDebug_print(true);

        boolean load_success = ci.load_class_file();
        is.close();

        if(!load_success) throw new PlcException("dump", "Load failure");
	}


    private static RandomAccessFile findFile(String fileName) {
        RandomAccessFile raf = null;

        String tryName = fileName;
        if (!tryName.endsWith(".pc")) tryName += ".pc";

        try {
            raf = new RandomAccessFile(tryName, "r");
            return raf;
        }
        catch (FileNotFoundException ex) { /* fall through */ }

        for (String path : paths) {
            File tryFile = new File(path, tryName);
            try {
                raf = new RandomAccessFile(tryFile, "r");
                return raf;
            }
            catch (FileNotFoundException ex) { /* fall through */ }
        }

        return null;
    }
}
