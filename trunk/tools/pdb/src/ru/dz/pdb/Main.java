package ru.dz.pdb;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.logging.Logger;

import javax.swing.UIManager;

import ru.dz.pdb.debugger.ClassMap;
import ru.dz.pdb.phantom.ObjectHeader;
import ru.dz.pdb.phantom.ObjectRef;
import ru.dz.pdb.ui.InspectorFrame;

//import ru.dz.gardemarine.ui.logger.LogWindowLogHandler;


/**
 * Phantom object debugger main class.
 * @author dz
 */
public class Main {
	private static final Logger log = Logger.getLogger(Main.class.getName()); 

	private static HostConnector hc;
	private static ClassMap cmap;

	public static HostConnector getHc() {
		return hc;
	}
	
	// --------------------------------------------------------------------
	// Main
	// --------------------------------------------------------------------
    
    /**
     * Well. Pdb main.
     * @param args
     * @throws IOException 
     * @throws UnknownHostException 
     * @throws CmdException 
     */
    public static void main(String[] args) throws UnknownHostException, IOException, CmdException {
		try { 
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
			} 
		catch (Exception e) {/* ignore inability to set l&f */}
		
		{
			Logger rootLogger = Logger.getLogger(""); // root	
			// TODO fixme
			//rootLogger.addHandler(new LogWindowLogHandler(db.getLogWindow()));
		}
		
		log.severe("Starting");

		hc = new HostConnector();
		
		long start = hc.cmdGetPoolAddress();
		System.out.println(String.format("Main.main() pool start 0x%X", start) );
		inspectObject( start );
		//InspectorFrame inspectorFrame = new InspectorFrame(start);
		
	}

    // Get object from attached host
	public static ObjectHeader getPhantomObject(long address) {
		byte[] data;
		try {
			data = hc.cmdGetObject(address);
		} catch (CmdException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return null;
		}
		return new ObjectHeader(data, address);		
	}


	public static void inspectObject( long address )
	{
		InspectorFrame inspectorFrame = new InspectorFrame(address);
	}

	public static void getPhantomClass(ObjectRef classRef) {
		cmap.get(classRef);
	}
	
	public static void getPhantomClass(long address) {
		cmap.get(address);
	}

}
