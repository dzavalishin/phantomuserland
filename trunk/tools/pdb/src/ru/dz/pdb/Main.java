package ru.dz.pdb;

import java.beans.ExceptionListener;
import java.beans.XMLDecoder;
import java.beans.XMLEncoder;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.UnknownHostException;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.UIManager;
import javax.swing.filechooser.FileNameExtensionFilter;

import ru.dz.pdb.debugger.ClassMap;
import ru.dz.pdb.phantom.ObjectHeader;
import ru.dz.pdb.phantom.ObjectRef;
import ru.dz.pdb.ui.InspectorFrame;
import ru.dz.pdb.ui.MainFrame;

//import ru.dz.gardemarine.ui.logger.LogWindowLogHandler;


/**
 * Phantom object debugger main class.
 * @author dz
 */
public class Main {
	private static final Logger log = Logger.getLogger(Main.class.getName()); 

	private static HostConnector hc;
	private static ClassMap cmap;

	private static MainFrame mainFrame;

	private static long objectSpaceStart;

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

		mainFrame = new MainFrame();

		objectSpaceStart = hc.cmdGetPoolAddress();

		inspectRootObject();
	}

	// Get object from attached host
	public static ObjectHeader getPhantomObject(long address) 
	{
		if( address < objectSpaceStart )
		{
			System.out.println(String.format("Main.getPhantomObject() address is below persistent space: 0x%X", address) );
			return null;
		}

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

	public static ObjectHeader getPhantomObject(ObjectRef o)
	{
		return getPhantomObject(o.getDataAddr());
	}

	public static void inspectObject( long address )
	{
		if( address < objectSpaceStart )
		{
			System.out.println(String.format("Main.inspectObject() address is below persistent space: 0x%X", address) );
			return;
		}
		InspectorFrame inspectorFrame = new InspectorFrame(address);
	}

	public static void inspectRootObject() 
	{
		System.out.println(String.format("Main.main() pool start 0x%X", objectSpaceStart) );
		inspectObject( objectSpaceStart );
	}



	public static void getPhantomClass(ObjectRef classRef) {
		cmap.get(classRef);
	}

	public static void getPhantomClass(long address) {
		cmap.get(address);
	}

	public static String getPhantomString(ObjectRef pString) {
		if(pString == null) return "(null)";

		ObjectHeader s = getPhantomObject(pString);

		if(s == null) return "(null)";

		return s.getAsString();
	}

	public static void doQuit() 
	{
		hc.disconnect();
		System.exit(0);		
	}

	@Override
	protected void finalize() throws Throwable {
		hc.disconnect();
		super.finalize();
	}

	// --------------------------------------------------------------------
	// Project
	// --------------------------------------------------------------------

	private static final String NEW_SFX = ".new"; //$NON-NLS-1$
	private static final String OLD_SFX = ".old"; //$NON-NLS-1$

	private static Project project = new Project();

	public static void openProject() 
	{
		if(!selectProjectFile("Open project"))
			return;

		FileInputStream fin = null;
		try {
			fin = new FileInputStream(project.getProjectFileName());
		} catch (FileNotFoundException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
			JOptionPane.showMessageDialog(null,"Load error "+e1); //$NON-NLS-1$
			return;
		}

		BufferedInputStream bin = new BufferedInputStream(fin,256*1024);

		ExceptionListener eListener = new ExceptionListener() {

			@Override
			public void exceptionThrown(Exception e) {
				log.log(Level.SEVERE,"Config load exception",e);

			}
		};

		//XMLDecoder decoder = new XMLDecoder(bin,getConstantState(),eListener,gcl);
		XMLDecoder decoder = new XMLDecoder(bin,null,eListener);

		// http://www.ftponline.com/javapro/2004_09/magazine/features/kgauthier/page5.aspx
		// recommends not to setOwner, 'cause it's too late.
		//decoder.setOwner(getConstantState()); 

		// Don't try lo load ones after some is failed.
		try {
			project = (Project) decoder.readObject();
		} catch (Throwable e) {
			log.log(Level.SEVERE,"Project load error",e); //$NON-NLS-1$
		}

		decoder.close();
		try {
			bin.close();
		} catch (IOException e) {
			log.log(Level.SEVERE,"Project file close error",e); //$NON-NLS-1$
		}
	}

	public static void saveProjectAs()  {
		if(project == null )
			project = new Project();

		if( (!selectProjectFile("Save project as")) || (project.getProjectFileName() == null ))
			return;
		
		saveProject();
	}
	
	public static void saveProject()  {
		if(project == null )
			project = new Project();

		if(project.getProjectFileName() == null )
		{
			if( (!selectProjectFile("Save project as")) || (project.getProjectFileName() == null ) )
				return;
		}
		

		File tmp = new File(project.getProjectFileName()+NEW_SFX);
		FileOutputStream fout;
		try {
			fout = new FileOutputStream(tmp);
		} catch (FileNotFoundException e1) {
			JOptionPane.showMessageDialog(null,"Save error "+e1); //$NON-NLS-1$
			return;
		}
		BufferedOutputStream bout = new BufferedOutputStream(fout, 128*1024);
		XMLEncoder encoder = new XMLEncoder(bout);

		//encoder.setOwner(getConstantState());

		encoder.setExceptionListener(new ExceptionListener() {
			public void exceptionThrown(Exception exception) {
				exception.printStackTrace();
			}
		});

		encoder.writeObject(project);

		encoder.flush();
		encoder.close();

		try {
			bout.close();
			File killOld = new File(project.getProjectFileName()+OLD_SFX);
			killOld.delete();

			File old = project.getProjectFileName();
			old.renameTo(new File(project.getProjectFileName()+OLD_SFX));
			tmp.renameTo(project.getProjectFileName());

			JOptionPane.showMessageDialog(null,"Project saved"); //$NON-NLS-1$


		} catch (IOException e) {
			log.log(Level.SEVERE,"Save error ",e); //$NON-NLS-1$
			JOptionPane.showMessageDialog(null,"Save error "+e); //$NON-NLS-1$
		}


	}

	private static boolean selectProjectFile(String dialogTitle) {
		JFileChooser chooser = new JFileChooser();
		FileNameExtensionFilter filter = new FileNameExtensionFilter(
				"Project files", "pd", "xml");
		chooser.setFileFilter(filter);
		chooser.setCurrentDirectory(new File("."));
		chooser.setDialogTitle(dialogTitle);
		
		chooser.setSelectedFile(project.getProjectFileName());
		
		int returnVal = chooser.showOpenDialog(null);

		if(returnVal == JFileChooser.APPROVE_OPTION) 
			project.setProjectFileName(new File(chooser.getSelectedFile().getName()));
		
		return returnVal == JFileChooser.APPROVE_OPTION;
	}

}
