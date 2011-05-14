package ru.dz.pdb.ui;

import javax.swing.JFrame;

import ru.dz.pdb.CmdException;
import ru.dz.pdb.Main;

public class InspectorFrame extends JFrame {
	private long phantomObjectAddress;
	private byte[] object;

	public InspectorFrame(long phantomObjectAddress) {
		this.phantomObjectAddress = phantomObjectAddress;
		setTitle( String.format("Object @0x%x",phantomObjectAddress) );
		
		try {
			object = Main.getHc().cmdGetObject(phantomObjectAddress);
		} catch (CmdException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			object = new byte[0];
		}
		
		
	}
	

}
