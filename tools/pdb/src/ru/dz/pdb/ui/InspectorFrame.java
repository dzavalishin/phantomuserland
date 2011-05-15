package ru.dz.pdb.ui;

import java.awt.Container;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Panel;

import javax.swing.JFrame;
import javax.swing.JLabel;

import ru.dz.pdb.CmdException;
import ru.dz.pdb.Main;
import ru.dz.pdb.phantom.InvalidObjectOperationException;
import ru.dz.pdb.phantom.ObjectHeader;
import ru.dz.pdb.ui.bits.RefButton;

public class InspectorFrame extends JFrame {
	private long phantomObjectAddress;
	private byte[] object;
	private ObjectHeader parsed;

	public InspectorFrame(long phantomObjectAddress) {
		this.phantomObjectAddress = phantomObjectAddress;
		setTitle( String.format("Object @0x%x",phantomObjectAddress) );
		
		try {
			object = Main.getHc().cmdGetObject(phantomObjectAddress);
			parsed = new ObjectHeader(object,phantomObjectAddress);
			//Main.getPhantomClass(parsed.getClassRef());
		} catch (CmdException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			
			object = new byte[0];
		
		}
		
		populateMe();
		
		pack();
		setVisible(true);
	}

	private void populateMe() {
	
		Container contentPane = getContentPane();
		
		contentPane.setLayout(new GridBagLayout());
		
		GridBagConstraints gbc = new GridBagConstraints();
		gbc.weightx = gbc.weighty = 1;

		gbc.gridx = 0;
		gbc.gridy = GridBagConstraints.RELATIVE;	
		Panel topPanel = new Panel(new GridBagLayout());
		contentPane.add(topPanel, gbc);
		poulateTopPanel(topPanel);
		
		//gbc.gridx = 0;
		//gbc.gridy = 1;	
		Panel mainPanel = new Panel(new GridBagLayout());
		contentPane.add(mainPanel, gbc);
		poulateMainPanel(mainPanel);
		
	}

	private void poulateMainPanel(Panel mainPanel) {
		// TODO Auto-generated method stub
		
	}

	private void poulateTopPanel(Panel panel) {
		GridBagConstraints gbc = new GridBagConstraints();
		gbc.weightx = gbc.weighty = 1;

		gbc.gridx = GridBagConstraints.RELATIVE;
		gbc.gridy = 0;	
		
		if(parsed == null)
		{
			panel.add( new JLabel("Invalid object"), gbc );
			return;
		}

		//panel.add( new JLabel("Object@"+Long.toHexString(phantomObjectAddress)+": "), gbc );
		
		
		if(parsed.isInternal())
			panel.add( new JLabel("Internal, da: "+parsed.getDaSize()), gbc );
		else
		{
			try {
				panel.add( new JLabel("Refs: "+parsed.getDaRefsCount()), gbc );
			} catch (InvalidObjectOperationException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		panel.add( new JLabel("Flags: "+parsed.getFlagsList()), gbc );
		
		//panel.add( new JLabel(parsed.isInternal() ? "Internal" : "Normal"), gbc );
		
		panel.add( new RefButton(parsed.getClassRef(),"Class"), gbc );
		panel.add( new RefButton(parsed.getObjectSatellites(),"Sat"), gbc );
		
	}
	

}
