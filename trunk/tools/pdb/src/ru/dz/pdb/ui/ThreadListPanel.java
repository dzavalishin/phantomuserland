package ru.dz.pdb.ui;

import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.table.AbstractTableModel;

import ru.dz.pdb.CmdException;
import ru.dz.pdb.Main;
import ru.dz.pdb.phantom.ObjectRef;
import ru.dz.pdb.ui.ThreadListTableModel.ThreadInfo;

public class ThreadListPanel extends JTable {

	//private ThreadListTableModel dm = new ThreadListTableModel();
	//private List<Integer> threadList = new LinkedList<Integer>();
	//private TableColumnModel cm = new ThreadListTableColumnModel();

	
	public ThreadListPanel() {
		//JTable t = new JTable(dm,cm);
		//super(dm);
		super(new ThreadListTableModel());
		//add(t);
		/*		
		try {
			threadList = Main.getThreadList();
		} catch (CmdException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		*/
		//Dimension preferredSize = new Dimension(300, 180);
		//t.setPreferredSize(preferredSize);

		/*
		TableColumn c1 = new TableColumn();
		c1.setHeaderValue("TID");
		t.addColumn(c1 );
		*/
		
		setFillsViewportHeight(true);

		setShowGrid(true);
		//t.setAutoCreateColumnsFromModel(true);
		//t.setRowSelectionAllowed(true);
		
	}
	
	

}


class ThreadListTableModel extends AbstractTableModel
{
	private List<Integer> threadList = new LinkedList<Integer>();
	private Map<Integer,ThreadInfo> threadInfoList = new TreeMap<Integer,ThreadInfo>();
	
	public ThreadListTableModel() {
		try {
			threadList = Main.getThreadList();
		} catch (CmdException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	private static final String [] columnNames = {"tid","ref"};

	
	@Override
	public int getColumnCount() { return columnNames.length; }

	@Override
	public String getColumnName(int col) {
		//return "tid";
		//System.out.println("ThreadListPanel.ThreadListTableModel.getColumnName()"+col);
		return columnNames[col];
	}

	@Override
	public int getRowCount() {
		//System.out.println("ThreadListPanel.ThreadListTableModel.getRowCount() = "+threadList.size());
		return threadList.size();
	}

	@Override
	public Object getValueAt(int rowIndex, int columnIndex) {
		if(columnIndex == 0)
			return threadList.get(rowIndex);

		ThreadInfo info = threadInfoList.get(rowIndex);
		if(info == null)
		{
			//info = getThreadInfo(threadList.get(rowIndex));
			info = new ThreadInfo(threadList.get(rowIndex));
			threadInfoList.put(rowIndex,info);
		}
		
		switch(columnIndex)
		{
		case 1:
			return info.getRef();
		}
		
		return null;
	}

	@Override
	public boolean isCellEditable(int rowIndex, int columnIndex) {
		// no way
		return false;
	}


	/*
	@Override
	public Class<?> getColumnClass(int columnIndex) {
		switch(columnIndex)
		{
		case 0:
			return Integer.class;
		}
		return null;
	}
	
	@Override
	public void addTableModelListener(TableModelListener l) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void removeTableModelListener(TableModelListener l) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setValueAt(Object aValue, int rowIndex, int columnIndex) {
		// ignore			
	}
	*/
	
	
	class ThreadInfo
	{
		private final int tid;

		public ThreadInfo(int tid) {
			this.tid = tid;
			
		}

		public ObjectRef getRef() {
			// TODO Auto-generated method stub
			return null;
		}
	}
}
