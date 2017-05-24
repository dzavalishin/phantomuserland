package ru.dz.pdb.phantom;

import java.awt.GridBagConstraints;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import javax.swing.JLabel;
import javax.swing.JPanel;

import phantom.data.ObjectRef;
import ru.dz.pdb.ui.bits.RefButton;

public class ArrayObject implements IInternalObject {

	
	private ObjectRef page;
	private int page_size;
	private int used_slots;

	public ArrayObject(ObjectHeader oh) throws InvalidObjectOperationException {
		if( (oh.getObjectFlags() & ObjectHeader.PHANTOM_OBJECT_STORAGE_FLAG_IS_DECOMPOSEABLE) == 0 )
			throw new InvalidObjectOperationException("Not decomposable array");

		if( (oh.getObjectFlags() & ObjectHeader.PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE) == 0 )
			throw new InvalidObjectOperationException("Not resizeable array");

		loadMe(oh.getDaData());
	}
	
	private void loadMe(byte[] data)
	{
		ByteBuffer bb = ByteBuffer.wrap(data);
		bb.order(ByteOrder.LITTLE_ENDIAN);

		page = new ObjectRef(bb);
		page_size = bb.getInt();
		used_slots= bb.getInt();
	}
	
	@Override
	public void populatePanel(JPanel panel, GridBagConstraints gbc) {
		panel.add( new JLabel("Array pg size: \t"+page_size+""), gbc );
		panel.add( new JLabel("Used slots:    \t"+used_slots+""), gbc );
		panel.add( new RefButton(page,"Page"), gbc );				
	}

}
