package ru.dz.pdb.ui.bits;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.beans.PropertyChangeListener;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.JButton;

import ru.dz.pdb.Main;
import ru.dz.pdb.phantom.ObjectRef;

/**
 * Opens ref (object inspector) on press, shows ref in tip.
 * @author dz
 *
 */
public class RefButton extends JButton {

	private long address;

	public RefButton(ObjectRef o, String buttonText) {
		address = o.getDataAddr();
		//setSize(new Dimension(40, 36));
		loadMe(buttonText);
		//setMinimumSize(new Dimension(40, 36));
		validate();
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
	}

	protected void openRef() {
		Main.inspectObject(address);		
	}

}
