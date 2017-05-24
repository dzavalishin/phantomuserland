package ru.dz.pdb.ui;

import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.swing.JTable;
import javax.swing.table.AbstractTableModel;

import phantom.data.ObjectRef;
import ru.dz.pdb.CmdException;
import ru.dz.pdb.Main;
import ru.dz.pdb.debugger.ClassMap;
import ru.dz.pdb.phantom.ClassObject;

public class ClassListPanel extends JTable {
	protected static final Logger log = Logger.getLogger(ClassListPanel.class.getName()); 


	public ClassListPanel() {
		super(new ClassListTableModel());

		setFillsViewportHeight(true);

		setShowGrid(true);

		//getColumnModel().getColumn(0).setPreferredWidth(16); 
		//getColumnModel().getColumn(0).setResizable(false);
	}


	public void reload() {
		ClassListTableModel m = (ClassListTableModel)getModel();
		m.reload();
		m.fireTableDataChanged();
	}



}


class ClassListTableModel extends AbstractTableModel
{
	protected static final Logger log = Logger.getLogger(ClassListTableModel.class.getName()); 

	private List<ClassObject> threadList = new LinkedList<ClassObject>();
	//private Map<Integer,ClassInfo> threadInfoList = new TreeMap<Integer,ClassInfo>();

	public ClassListTableModel() {
		reload();
	}

	void reload()
	{
		ClassMap classMap = Main.getClassMap();
		Collection<ClassObject> list = classMap.getList();
		threadList = new ArrayList<ClassObject>(list);
	}
	
	private static final String [] columnNames = {"Name","SysId","Info"};


	@Override
	public int getColumnCount() { return columnNames.length; }

	@Override
	public String getColumnName(int col) {		return columnNames[col];	}

	@Override
	public int getRowCount() {		return threadList.size();	}

	@Override
	public Object getValueAt(int rowIndex, int columnIndex) {

		switch(columnIndex)
		{
		case 0:			return threadList.get(rowIndex).getClassName();
		//case 2:			return new RefButton(info.getRef(),"Ref");			
		case 1:			return threadList.get(rowIndex).getSysId();
		case 2:			return threadList.get(rowIndex).toString();
		}

		return null;
	}

	@Override
	public boolean isCellEditable(int rowIndex, int columnIndex) {
		// no way
		return false;
	}


/*
	class ClassInfo
	{
		//private static final Logger log = Logger.getLogger(ThreadInfo.class.getName()); 

		private final int tid;
		private String sinfo = "";
		private String name = "?";
		private ObjectRef object;
		private boolean running;

		public ClassInfo(int tid) {
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
	*/
}
