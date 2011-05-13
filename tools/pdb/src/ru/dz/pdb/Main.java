package ru.dz.pdb;

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


	
	// --------------------------------------------------------------------
	// Main
	// --------------------------------------------------------------------
    
    /**
     * Well. Pdb main.
     * @param args
     */
    public static void main(String[] args) {
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

		InspectorFrame inspectorFrame = new InspectorFrame(0x80000000);
		
	}


	

}
