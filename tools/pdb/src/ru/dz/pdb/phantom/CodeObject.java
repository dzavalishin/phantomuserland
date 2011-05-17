package ru.dz.pdb.phantom;

import java.awt.GridBagConstraints;

import javax.swing.JPanel;

public class CodeObject implements IInternalObject {
	private byte [] data;

	public CodeObject(ObjectHeader oh) throws InvalidObjectOperationException {
		if( (oh.getObjectFlags() & ObjectHeader.PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE) == 0 )
			throw new InvalidObjectOperationException("Not a code object");

		data = oh.getDaData();
	}
	
	@Override
	public void populatePanel(JPanel p, GridBagConstraints gbc) {
		// TODO Auto-generated method stub

	}

}
