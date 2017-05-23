package ru.dz.pdb.phantom;

import java.awt.GridBagConstraints;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import javax.swing.JLabel;
import javax.swing.JPanel;

import phantom.data.ObjectRef;
import ru.dz.pdb.ui.bits.RefButton;

public class CallFrameObject implements IInternalObject {

	private int codePtr; // mem address
	private int ip = 0;
	private int ipMax = 0;
	private int ordinal;
	
	private ObjectRef iStack;
	private ObjectRef oStack;
	private ObjectRef eStack;

	private ObjectRef thisp;
	private ObjectRef prevFrame;

	public CallFrameObject(ObjectHeader oh) throws InvalidObjectOperationException {
		if( (oh.getObjectFlags() & ObjectHeader.PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME) == 0 )
			throw new InvalidObjectOperationException("No call frame flag");

		loadMe(oh.getDaData());
	}
	
	private void loadMe(byte[] data)
	{
		ByteBuffer bb = ByteBuffer.wrap(data);
		bb.order(ByteOrder.LITTLE_ENDIAN);

		iStack = new ObjectRef(bb);
		oStack = new ObjectRef(bb);
		eStack = new ObjectRef(bb);
		
		ipMax = bb.getInt();
		codePtr = bb.getInt();
		ip = bb.getInt();
		
		thisp = new ObjectRef(bb);
		prevFrame = new ObjectRef(bb);

		ordinal = bb.getInt();
	}
	
	@Override
	public void populatePanel(JPanel panel, GridBagConstraints gbc) {
		panel.add( new JLabel("Code ptr: \t"+codePtr+""), gbc );
		panel.add( new JLabel("IP:       \t"+ip+""), gbc );
		panel.add( new JLabel("IP max:   \t"+ipMax+""), gbc );
		panel.add( new JLabel("Ordinal:  \t"+ordinal+""), gbc );
		panel.add( new RefButton(iStack,"I stack"), gbc );				
		panel.add( new RefButton(oStack,"O stack"), gbc );				
		panel.add( new RefButton(eStack,"E stack"), gbc );				
		panel.add( new RefButton(thisp,"This"), gbc );				
		panel.add( new RefButton(prevFrame,"Prev Frame"), gbc );				
	}

}
