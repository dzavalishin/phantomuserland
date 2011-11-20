package ru.dz.pdb.phantom;

import java.awt.GridBagConstraints;
import java.io.UnsupportedEncodingException;

import javax.swing.JLabel;
import javax.swing.JPanel;

public class StringObject implements IInternalObject {
	private byte [] data;

	public StringObject(ObjectHeader oh) throws InvalidObjectOperationException {
		if( (oh.getObjectFlags() & ObjectHeader.PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING) == 0 )
			throw new InvalidObjectOperationException("Not a string object");

		data = oh.getDaData();
	}
	
	@Override
	public void populatePanel(JPanel p, GridBagConstraints gbc) {
		try {
			p.add(new JLabel(new String(data,"UTF-8")),gbc);
		} catch (UnsupportedEncodingException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	

}
