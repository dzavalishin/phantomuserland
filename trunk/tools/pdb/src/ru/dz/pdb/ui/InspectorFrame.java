package ru.dz.pdb.ui;

import javax.swing.JFrame;

public class InspectorFrame extends JFrame {
	private int phantomObjectAddress;

	public InspectorFrame(int phantomObjectAddress) {
		this.phantomObjectAddress = phantomObjectAddress;
		setTitle( String.format("Object @0x%x",phantomObjectAddress) );
	}
	
	
}
