package ru.dz.pdb.ui;

import java.awt.Container;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.util.Vector;

import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSeparator;
import javax.swing.JTabbedPane;
import javax.swing.SwingConstants;

import phantom.data.ObjectRef;
import ru.dz.pdb.CmdException;
import ru.dz.pdb.Main;
import ru.dz.pdb.phantom.IKnownType;
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

		setLocationByPlatform(true);
		setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);

		validate();
		pack();
		setVisible(true);
	}

	private void populateMe() {

		Container contentPane = getContentPane();

		contentPane.setLayout(new GridBagLayout());

		GridBagConstraints gbc = new GridBagConstraints();
		gbc.weightx = 1;
		gbc.weighty = 0;

		gbc.gridx = 0;
		gbc.gridy = GridBagConstraints.RELATIVE;	

		gbc.anchor = GridBagConstraints.NORTHWEST;

		gbc.insets = new Insets(2, 2, 2, 2);
		//gbc.ipadx = 4;		gbc.ipady = 4;

		gbc.fill = GridBagConstraints.HORIZONTAL;

		JPanel topPanel = new JPanel(new GridBagLayout());
		contentPane.add(topPanel, gbc);
		poulateTopPanel(topPanel);

		gbc.fill = GridBagConstraints.BOTH;
		gbc.weightx = 1;
		gbc.weighty = 1;

		//gbc.gridx = 0;
		//gbc.gridy = 1;	
		JTabbedPane tabs = new JTabbedPane();

		{
			JPanel mainPanel = new JPanel(new GridBagLayout());		
			//Panel mainPanel = new Panel();		
			poulateMainPanel(mainPanel);


			JScrollPane scroll = new JScrollPane(mainPanel);
			scroll.setPreferredSize(new Dimension(450, 200));
			scroll.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);


			//scroll.getHorizontalScrollBar().setValue(0);

			tabs.addTab("Main", scroll);
		}

		{
			DumpPanel mainPanel = new DumpPanel(parsed.getDaData());		


			JScrollPane scroll = new JScrollPane(mainPanel);
			scroll.setPreferredSize(new Dimension(450, 200));
			scroll.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);

			tabs.addTab("Dump", scroll);
		}


		//contentPane.add(scroll, gbc); 
		contentPane.add(tabs, gbc); 

	}

	private void poulateTopPanel(JPanel panel) {
		GridBagConstraints gbc = new GridBagConstraints();
		gbc.weightx = gbc.weighty = 1;

		gbc.gridx = GridBagConstraints.RELATIVE;
		gbc.gridy = 0;	

		gbc.anchor = GridBagConstraints.NORTH;

		//gbc.fill = GridBagConstraints.BOTH;
		gbc.fill = GridBagConstraints.HORIZONTAL;

		if(parsed == null)
		{
			panel.add( new JLabel("Invalid object"), gbc );
			return;
		}

		//panel.add( new JLabel("Object@"+Long.toHexString(phantomObjectAddress)+": "), gbc );


		if(parsed.isInternal())
			panel.add( new JLabel(" Internal, da: "+parsed.getDaSize()), gbc );
		else
		{
			try {
				panel.add( new JLabel(" Slots: "+parsed.getDaRefsCount()), gbc );
			} catch (InvalidObjectOperationException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}

		panel.add( new JSeparator(SwingConstants.VERTICAL), gbc );

		int refCount = parsed.getRefCount();
		if(refCount == 0x7FFFFFFF)
			panel.add( new JLabel("| RefCnt: SAT"), gbc );
		else
			panel.add( new JLabel("| RefCnt: "+refCount), gbc );

		//panel.add( new JLabel(" | Flags: "+parsed.getFlagsList()+" | "), gbc );

		if(parsed.getObjectFlags() != 0) {
			//panel.add( new JLabel(" | Flags: "), gbc );
			panel.add( new JLabel(" | "), gbc );

			JLabel fl = new JLabel(parsed.getFlagsList());
			fl.setFont(new Font(fl.getFont().getFontName(),Font.BOLD,fl.getFont().getSize()));
			fl.setToolTipText("flags");
			panel.add( fl, gbc );

		}

		panel.add( new JLabel(" | "), gbc );

		panel.add( new JSeparator(SwingConstants.VERTICAL), gbc );

		//panel.add( new JLabel(parsed.isInternal() ? "Internal" : "Normal"), gbc );

		panel.add( new RefButton(parsed.getClassRef(),"Class"), gbc );
		panel.add( new RefButton(parsed.getObjectSatellites(),"Sat"), gbc );

		panel.add( new JLabel(" "), gbc );

	}

	private void poulateMainPanel(JPanel panel) 
	{
		GridBagConstraints gbc = new GridBagConstraints();
		gbc.weightx = gbc.weighty = 1;

		gbc.gridx = 0;
		gbc.gridy = GridBagConstraints.RELATIVE;	

		gbc.anchor = GridBagConstraints.NORTHWEST;

		gbc.insets = new Insets(2, 2, 0, 2);

		gbc.fill = GridBagConstraints.BOTH;
		//gbc.fill = GridBagConstraints.HORIZONTAL;

		if(parsed == null)
		{
			panel.add( new JLabel(""), gbc );
			return;
		}
		if(parsed.isInternal())
		{
			try {
				IKnownType avatar = parsed.getAvatar();
				avatar.populatePanel(panel, gbc);
			} catch (InvalidObjectOperationException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		else
		{
			// Regular (not internal) object
			try {
				int n = parsed.getDaRefsCount();
				Vector<ObjectRef> refs = parsed.getDaRefs();

				for( int i = 0; i < n; i++ )
				{
					panel.add( new RefButton(refs.get(i),"Ref "+i), gbc );				
				}
			} catch (InvalidObjectOperationException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}


}
