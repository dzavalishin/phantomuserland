package ru.dz.pdb.phantom;

import java.awt.GridBagConstraints;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import javax.swing.JLabel;
import javax.swing.JPanel;

import phantom.data.ObjectRef;
import ru.dz.pdb.ui.bits.RefButton;

public class ThreadObject implements IInternalObject {

	private int codePtr; // mem address
	private int ip = 0;
	private int ipMax = 0;
	
	private ObjectRef callFrame;
	private ObjectRef owner;
	private ObjectRef environment;

	public ThreadObject(ObjectHeader oh) throws InvalidObjectOperationException {
		if( (oh.getObjectFlags() & ObjectHeader.PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD) == 0 )
			throw new InvalidObjectOperationException("No thread flag");

		loadMe(oh.getDaData());
	}
	
	private void loadMe(byte[] data)
	{
		ByteBuffer bb = ByteBuffer.wrap(data);
		bb.order(ByteOrder.LITTLE_ENDIAN);

		codePtr = bb.getInt();

		ip = bb.getInt();
		ipMax = bb.getInt();
		
		callFrame = new ObjectRef(bb);
		owner = new ObjectRef(bb);
		environment = new ObjectRef(bb);
		
		// following fields are to be done
	}
	
	@Override
	public void populatePanel(JPanel panel, GridBagConstraints gbc) {
		panel.add( new JLabel("Code ptr: \t"+codePtr+""), gbc );
		panel.add( new JLabel("IP:       \t"+ip+""), gbc );
		panel.add( new JLabel("IP max:   \t"+ipMax+""), gbc );
		panel.add( new RefButton(callFrame,"Call frame"), gbc );				
		panel.add( new RefButton(owner,"Owner"), gbc );				
		panel.add( new RefButton(environment,"Environment"), gbc );				
	}

}
