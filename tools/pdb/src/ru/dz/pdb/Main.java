package ru.dz.pdb;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.logging.Logger;

import javax.swing.UIManager;

import ru.dz.pdb.ui.InspectorFrame;

//import ru.dz.gardemarine.ui.logger.LogWindowLogHandler;


/**
 * Phantom object debugger main class.
 * @author dz
 */
public class Main {
	private static final Logger log = Logger.getLogger(Main.class.getName()); 

	private static HostConnector hc;

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
		InspectorFrame inspectorFrame = new InspectorFrame(start);
		
	}


	

}
