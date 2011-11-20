package ru.dz.pdb.phantom;

import java.awt.GridBagConstraints;
import java.util.ArrayList;

import javax.swing.JLabel;
import javax.swing.JPanel;

public class IStackObject extends AbstractStackObject {

	public IStackObject(ObjectHeader oh) throws InvalidObjectOperationException {
		super(oh);
		loadMe();
	}

	private ArrayList<Integer> els = new ArrayList<Integer>();
	
	private void loadMe() 
	{
		commonLoadMe(bb);
		for(int i = 0; i < freeCellPtr; i++)
		{
			els.add( bb.getInt() );
		}
	}

	@Override
	public void populatePanel(JPanel panel, GridBagConstraints gbc) {
		commonPopulatePanel(panel, gbc);
		int i = 0;
		for( Integer el : els )
		{
			String ename  = "El " + i++;
			panel.add( new JLabel(ename + " = " + el.intValue()+""), gbc );
		}
				
	}

}
