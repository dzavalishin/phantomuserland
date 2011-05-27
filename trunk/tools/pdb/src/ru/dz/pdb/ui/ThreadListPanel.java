package ru.dz.pdb.ui;

import java.awt.Dimension;
import java.util.LinkedList;
import java.util.List;

import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.table.AbstractTableModel;

import ru.dz.pdb.CmdException;
import ru.dz.pdb.Main;

public class ThreadListPanel extends JPanel {

	private ThreadListTableModel dm = new ThreadListTableModel();
	private List<Integer> threadList = new LinkedList<Integer>();
	//private TableColumnModel cm = new ThreadListTableColumnModel();

	private static final String [] columnNames = {"tid"};

	
	public ThreadListPanel() {
		//JTable t = new JTable(dm,cm);
		JTable t = new JTable(dm);
		add(t);
				
		try {
			threadList = Main.getThreadList();
		} catch (CmdException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		//Dimension preferredSize = new Dimension(300, 180);
		//t.setPreferredSize(preferredSize);

		/*
		TableColumn c1 = new TableColumn();
		c1.setHeaderValue("TID");
		t.addColumn(c1 );
		*/
		
		t.setFillsViewportHeight(true);

		t.setShowGrid(true);
		t.setAutoCreateColumnsFromModel(true);
		t.setRowSelectionAllowed(true);
		
	}
	
	
	class ThreadListTableModel extends AbstractTableModel
	{
		
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
			return threadList.get(rowIndex);
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
	}

}
