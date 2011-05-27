package ru.dz.pdb.ui;

import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.table.AbstractTableModel;

import ru.dz.pdb.CmdException;
import ru.dz.pdb.Main;
import ru.dz.pdb.phantom.ObjectRef;
import ru.dz.pdb.ui.ThreadListTableModel.ThreadInfo;
import ru.dz.pdb.ui.bits.RefButton;

public class ThreadListPanel extends JTable {
	protected static final Logger log = Logger.getLogger(ThreadListPanel.class.getName()); 

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

		getColumnModel().getColumn(0).setPreferredWidth(16); 
		getColumnModel().getColumn(2).setPreferredWidth(16);
		getColumnModel().getColumn(3).setPreferredWidth(32);
		
		
		getColumnModel().getColumn(0).setResizable(false);
		//getColumnModel().getColumn(1).setResizable(false);
		getColumnModel().getColumn(2).setResizable(false);
		getColumnModel().getColumn(3).setResizable(false);

		getColumnModel().getColumn(4).setResizable(true);
	}



}


class ThreadListTableModel extends AbstractTableModel
{
	protected static final Logger log = Logger.getLogger(ThreadListTableModel.class.getName()); 

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

	private static final String [] columnNames = {"TID","Name","Ref","State","Info"};


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
		case 1:			return info.getName();			
		//case 2:			return new RefButton(info.getRef(),"Ref");			
		case 2:			return info.getRef();
		case 3:			return info.isRunning() ? "RUN" : "stop";
		case 4:			return info.toString();
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
		//private static final Logger log = Logger.getLogger(ThreadInfo.class.getName()); 

		private final int tid;
		private String sinfo = "";
		private String name = "?";
		private ObjectRef object;
		private boolean running;

		public ThreadInfo(int tid) {
			this.tid = tid;
			try {
				sinfo  = Main.getThreadExtraInfo(tid);
			} catch (CmdException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			String[] ss = sinfo.split(",");
			for( String el : ss )
			{
				System.out
				.println("ThreadListTableModel.ThreadInfo.ThreadInfo() el='"+el+";");

				int eqpos = el.indexOf("=");
				if(eqpos < 0)
				{
					log.log( Level.SEVERE, "el syntax: "+el);
					continue;
				}

				String key = el.substring(0, eqpos);
				String val = el.substring(eqpos+1);

				if(key.equals("name"))					name  = val;
				if(key.equals("object"))				
				{
					int addr = Integer.parseInt(val,16);
					object = new ObjectRef(addr,0);
				}
				if(key.equals("status"))				
				{
					running = !val.equals("stop");
				}
			}
		}


		public ObjectRef getRef() { return object; }
		public Object getName() { return name; }
		public boolean isRunning() { return running; }

		@Override
		public String toString() {
			return sinfo;
		}
	}
}
