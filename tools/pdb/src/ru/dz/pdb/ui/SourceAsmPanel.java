package ru.dz.pdb.ui;

import java.io.IOException;
import java.util.Map;
import java.util.TreeMap;
import java.util.logging.Logger;

import javax.swing.JTable;
import javax.swing.table.AbstractTableModel;

import phantom.data.ObjectRef;
import ru.dz.pdb.phantom.ClassObject;
import ru.dz.pdb.phantom.FileProgramSource;
import ru.dz.pdb.phantom.IProgramSource;
import ru.dz.pdb.phantom.PhantomClassNotFoundException;

public class SourceAsmPanel extends JTable {
	protected static final Logger log = Logger.getLogger(SourceAsmPanel.class.getName()); 

	
	public SourceAsmPanel() {
		super(new SourceAsmTableModel());
		setFillsViewportHeight(true);
		setShowGrid(true);
		//t.setRowSelectionAllowed(true);

		getColumnModel().getColumn(0).setPreferredWidth(16); 

		getColumnModel().getColumn(0).setResizable(false);
	}

	private SourceAsmTableModel m() { return (SourceAsmTableModel)getModel(); }
	
	public void reload() {
		m().reload();
		m().fireTableDataChanged();
	}


	public void setClassMethod(ClassObject currentClass, int currentMethod )
	{
		m().setClassMethod(currentClass, currentMethod );
	}

	public void setAddress(int a )
	{
		m().setAddress( a );
	}
	
}


class SourceAsmTableModel extends AbstractTableModel
{
	protected static final Logger log = Logger.getLogger(SourceAsmTableModel.class.getName()); 

	public SourceAsmTableModel() {
		reload();
	}

	void reload()
	{
	}

	private ClassObject		currentClass;
	private int				currentMethod;
	private int				currentLine;
	private int				currentAddress;

	private String			currentClassName = "?";
	
	private IProgramSource 	source;
	private boolean 		haveSource = false; 

	private boolean 		haveBinary = false; 
	
	Map<Integer,Integer>	line2ip = new TreeMap<Integer, Integer>();
	Map<Integer,Integer>	ip2line = new TreeMap<Integer, Integer>();
	
	public void setClassMethod(ClassObject currentClass, int currentMethod )
	{
		this.currentClass = currentClass;
		this.currentMethod = currentMethod;
		
		if(currentClass == null)
		{
			currentClassName = "(null)";
			return;
		}
		
		currentClassName = currentClass.getClassName();
		
		loadIp2LineMap(currentClass.getIp2LineMapRef());
		loadSource();
		loadBinary();
	}

	private void loadBinary() {
		haveBinary = false;
		// TODO Auto-generated method stub
		
	}

	private void loadSource() {
		source = null;
		haveSource = false;
		try {
			source = new FileProgramSource(currentClassName);
			haveSource = true;
			
			if(line2ip.isEmpty())
			{
				int linesCount = source.getLinesCount();
				
				for(int i = 0; i < linesCount; i++ )
				{
					line2ip.put(i, 0);
					ip2line.put(0, i);
				}
			}
			
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (PhantomClassNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}

	private void loadIp2LineMap(ObjectRef ip2LineMapRef) {
		line2ip = new TreeMap<Integer, Integer>();
		ip2line = new TreeMap<Integer, Integer>();
		// TODO load
	}
	
	public void setAddress(int a ) { currentAddress = a; mapIp2Line(); }
	
	private void mapIp2Line() {
		currentLine = 0;		
	}

	
	
	private static final String [] columnNames = {"Addr","Line","Source"};


	@Override
	public int getColumnCount() { return columnNames.length; }

	@Override
	public String getColumnName(int col) { return columnNames[col]; }

	@Override
	public int getRowCount() { return line2ip.size(); }

	@Override
	public Object getValueAt(int rowIndex, int columnIndex) {

		switch(columnIndex)
		{
		case 1:			return 0;			
		case 2:			return 0;
		}

		return null;
	}

	@Override
	public boolean isCellEditable(int rowIndex, int columnIndex) {
		// no way
		return false;
	}

	@Override
	public Class<?> getColumnClass(int columnIndex) {
		switch(columnIndex)
		{
		//case 3:			return ImageIcon.class;
		}
		return Object.class;
	}


}
