package ru.dz.pdb.phantom;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.Rectangle;

import javax.swing.JComponent;
import javax.swing.JPanel;
import javax.swing.Scrollable;
import javax.swing.SwingConstants;

import ru.dz.pdb.ui.DumpPanel;
import ru.dz.pdb.ui.bits.HexView;

public class CodeObject implements IInternalObject, Scrollable {
	private byte [] data;
	private JComponent hexView;

	public CodeObject(ObjectHeader oh) throws InvalidObjectOperationException {
		if( (oh.getObjectFlags() & ObjectHeader.PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE) == 0 )
			throw new InvalidObjectOperationException("Not a code object");

		data = oh.getDaData();
	}
	
	@Override
	public void populatePanel(JPanel p, GridBagConstraints gbc) {
		hexView = new HexView(data);
		//hexView = new DumpPanel(data);

		//p.setSize(new Dimension(hexView.getWidth(),p.getHeight()));
		
		//p.get
		
		p.add(hexView,gbc);
	}

	@Override
	public Dimension getPreferredScrollableViewportSize() {
		return hexView.getPreferredSize();
	}

	@Override
	public int getScrollableBlockIncrement(Rectangle visibleRect,
			int orientation, int direction) {
		
		
		if(orientation == SwingConstants.HORIZONTAL) {
	        return 1;
	    } else {
			//int h = (int) ((visibleRect.getHeight()/hexView.getLineHeight()) * hexView.getLineHeight());
	    	int h = 16;
	    	return h;
	    }
	}

	@Override
	public boolean getScrollableTracksViewportHeight() {
		return false;
	}

	@Override
	public boolean getScrollableTracksViewportWidth() {
		return false;
	}

	@Override
	public int getScrollableUnitIncrement(Rectangle visibleRect,
			int orientation, int direction) {
		if(orientation == SwingConstants.HORIZONTAL) {
	        return 1;
	    } else {
	    	//return hexView.getLineHeight();
	    	return 16;
	    }
	}

}
