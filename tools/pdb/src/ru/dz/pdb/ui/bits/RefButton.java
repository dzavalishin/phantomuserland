package ru.dz.pdb.ui.bits;

import java.awt.event.ActionEvent;

import javax.swing.AbstractAction;
import javax.swing.JButton;

import phantom.data.ObjectRef;
import ru.dz.pdb.Main;

/**
 * Opens ref (object inspector) on press, shows ref in tip.
 * @author dz
 *
 */
public class RefButton extends JButton {

	private long address;

	public RefButton(ObjectRef o, String buttonText) {
		address = (o == null) ? 0 : o.getDataAddr();
		//setSize(new Dimension(40, 36));
		loadMe(buttonText);
		
		
		//setMinimumSize(new Dimension(40, 36));
		//validate();
	}

	public RefButton(long address, String buttonText) {
		this.address = address;
		loadMe(buttonText);
	}

	private void loadMe(String buttonText) {
		setAction(new AbstractAction() {
			public void actionPerformed(ActionEvent e) {
				openRef();				
			}});		
		setToolTipText(Long.toHexString(address));
		setText(buttonText);
		if( address == 0 )
			setEnabled(false);
}

	protected void openRef() {
		Main.inspectObject(address);		
	}

}
