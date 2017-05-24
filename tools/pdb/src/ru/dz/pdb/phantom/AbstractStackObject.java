package ru.dz.pdb.phantom;

import java.awt.GridBagConstraints;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import javax.swing.JLabel;
import javax.swing.JPanel;

import phantom.data.ObjectRef;
import ru.dz.pdb.ui.bits.RefButton;

public abstract class AbstractStackObject implements IInternalObject 
{
	protected ByteBuffer bb; 
	
	protected int pageSize;
	protected int currDaPtr = 0; // mem address
	protected int freeCellPtr = 0;
	
	protected ObjectRef root;
	protected ObjectRef curr;
	protected ObjectRef prev;
	protected ObjectRef next;

	public AbstractStackObject(ObjectHeader oh) throws InvalidObjectOperationException {
		if( (oh.getObjectFlags() & ObjectHeader.PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME) == 0 )
			throw new InvalidObjectOperationException("No stack frame flag");

		bb = ByteBuffer.wrap(oh.getDaData());
		bb.order(ByteOrder.LITTLE_ENDIAN);		
	}
	
	protected void commonLoadMe(ByteBuffer bb)
	{
		root = new ObjectRef(bb);
		curr = new ObjectRef(bb);
		prev = new ObjectRef(bb);
		next = new ObjectRef(bb);
		
		freeCellPtr = bb.getInt();
		pageSize = bb.getInt();

		currDaPtr = bb.getInt();	
	}
	
	protected void commonPopulatePanel(JPanel panel, GridBagConstraints gbc) 
	{
		panel.add( new JLabel("Page size: \t"+pageSize+""), gbc );
		panel.add( new JLabel("Da ptr:    \t"+currDaPtr+""), gbc );
		panel.add( new JLabel("Free cell: \t"+freeCellPtr+""), gbc );

		panel.add( new RefButton(root,"Root"), gbc );				
		panel.add( new RefButton(curr,"Curr"), gbc );				
		panel.add( new RefButton(prev,"Prev"), gbc );				
		panel.add( new RefButton(next,"Next"), gbc );				
	}

}
