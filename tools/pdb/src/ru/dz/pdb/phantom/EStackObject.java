package ru.dz.pdb.phantom;

import java.awt.GridBagConstraints;
import java.util.ArrayList;

import javax.swing.JLabel;
import javax.swing.JPanel;

import phantom.data.ObjectRef;
import ru.dz.pdb.ui.bits.RefButton;

public class EStackObject extends AbstractStackObject {

	public EStackObject(ObjectHeader oh) throws InvalidObjectOperationException {
		super(oh);
		loadMe();
	}

	private ArrayList<PvmExceptionHandler> els = new ArrayList<PvmExceptionHandler>();
	
	private void loadMe() 
	{
		commonLoadMe(bb);
		for(int i = 0; i < freeCellPtr; i++)
		{
			els.add( new PvmExceptionHandler() );
		}
	}

	@Override
	public void populatePanel(JPanel panel, GridBagConstraints gbc) {
		commonPopulatePanel(panel, gbc);
		int i = 0;
		for( PvmExceptionHandler e : els )
		{
			String ename  = "El " + i++;
			panel.add( new JLabel(ename + " jump = " + e.jump+""), gbc );
			panel.add( new RefButton(e.object,"Object"), gbc );				
		}
	}

	
	class PvmExceptionHandler
	{
		public ObjectRef	object;
		public int			jump;

		protected PvmExceptionHandler() 
		{
			object = new ObjectRef(bb);
			jump = bb.getInt();
		}
		
	}
	
}

