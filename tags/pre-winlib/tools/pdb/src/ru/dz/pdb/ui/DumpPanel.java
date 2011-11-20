package ru.dz.pdb.ui;

import java.awt.Font;
import java.util.logging.Logger;

import javax.swing.JTable;
import javax.swing.table.AbstractTableModel;


public class DumpPanel extends JTable {
	protected static final Logger log = Logger.getLogger(DumpPanel.class.getName()); 


	
	public DumpPanel(byte[] data) {
		super(new DumpTableModel(data));
		setFillsViewportHeight(true);
		setShowGrid(true);
		//t.setRowSelectionAllowed(true);

		setFont(new Font(Font.MONOSPACED,Font.PLAIN,12));
		
		getColumnModel().getColumn(0).setPreferredWidth(16); 
		getColumnModel().getColumn(0).setWidth(16); 
		//getColumnModel().getColumn(0).setResizable(false);

		getColumnModel().getColumn(1).setPreferredWidth(100); 
		//getColumnModel().getColumn(1).setResizable(true);

		//getColumnModel().getColumn(0).setPreferredWidth(16); 
		//getColumnModel().getColumn(2).setResizable(true);
	}

	private DumpTableModel m() { return (DumpTableModel)getModel(); }
	
	public void reload() {
		m().reload();
		m().fireTableDataChanged();
	}
	
}


class DumpTableModel extends AbstractTableModel
{
	private static final int N_CHARS_PER_LINE = 16;
	
	protected static final Logger log = Logger.getLogger(DumpTableModel.class.getName());
	private final byte[] data; 

	public DumpTableModel(byte [] data) {
		this.data = data;
		reload();
	}

	void reload()
	{
	}

	
	/**
	 * Convert a byte[] array to hex dump.
	 * 
	 * @return result String buffer in String format 
	 * @param in byte[] buffer to convert to string format
	 */

	static String byteArrayToHexString( byte in[], int start, int len ) 
	{
		int i;
		int fill = len;

		if (in == null || in.length <= 0)
			return null;

		if(start+len > in.length)
			len = in.length-start;
		
		StringBuilder out = new StringBuilder(in.length * 3);

		out.append(" ");
		for(i = start; i < start+len; i++)
		{
			if( (i % 4) == 0 )
				out.append(" ");
			out.append(String.format("%02X", in[i]));
		}

		for(; i < start+fill; i++) 
			out.append("  ");

		out.append(" ");

		return out.toString();
	}    

	/**
	 * Convert a byte[] array to chars.
	 * 
	 * @return result String buffer in String format 
	 * @param in byte[] buffer to convert to string format
	 */

	static String byteArrayToCharString( byte in[], int start, int len ) 
	{
		//byte ch;
		int i;
		int fill = len;

		if (in == null || in.length <= 0)
			return null;

		if(start+len > in.length)
			len = in.length-start;
		
		StringBuilder out = new StringBuilder(in.length);
		out.append(" ");

		for(i = start; i < start+len; i++) 
		{
			if( (i % 4) == 0 )
				out.append(" ");
			char c = (char)in[i];
			if( in[i] < 32) c = '.';
			out.append(c);
		}

		for(; i < start+fill; i++) 
			out.append(' ');
		
		out.append(" ");
		return out.toString();
	}    
	
	
	private static final String [] columnNames = {"Addr","Hex","Bytes"};


	@Override
	public int getColumnCount() { return columnNames.length; }

	@Override
	public String getColumnName(int col) { return columnNames[col]; }

	@Override
	public int getRowCount() { return data.length/N_CHARS_PER_LINE; }

	@Override
	public Object getValueAt(int rowIndex, int columnIndex) {

		switch(columnIndex)
		{
		case 0:			return String.format(" %04X ", rowIndex * N_CHARS_PER_LINE );
		case 1:			return byteArrayToHexString(data, rowIndex*N_CHARS_PER_LINE, N_CHARS_PER_LINE);			
		case 2:			return byteArrayToCharString(data, rowIndex*N_CHARS_PER_LINE, N_CHARS_PER_LINE);
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
