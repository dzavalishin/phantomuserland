package ru.dz.pdb.phantom;

import java.awt.GridBagConstraints;
import java.util.ArrayList;

import javax.swing.JPanel;

import phantom.data.ObjectRef;
import ru.dz.pdb.ui.bits.RefButton;

public class OStackObject extends AbstractStackObject {

	public OStackObject(ObjectHeader oh) throws InvalidObjectOperationException {
		super(oh);
		loadMe();
	}

	private ArrayList<ObjectRef> els = new ArrayList<ObjectRef>();
	
	private void loadMe() 
	{
		commonLoadMe(bb);
		for(int i = 0; i < freeCellPtr; i++)
		{
			els.add( new ObjectRef(bb) );
		}
	}

	@Override
	public void populatePanel(JPanel panel, GridBagConstraints gbc) {
		commonPopulatePanel(panel, gbc);
		int i = 0;
		for( ObjectRef r : els )
			panel.add( new RefButton(r,"El +" + i++), gbc );				
	}

}
