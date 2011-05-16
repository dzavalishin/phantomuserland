package ru.dz.pdb.misc;

import java.awt.Color;
import java.awt.Cursor;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;

import javax.swing.ImageIcon;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.SwingConstants;
import javax.swing.border.EtchedBorder;

public class AboutFrame extends JDialog {



	private AboutFrame() {
		//super("About "+Version.NAME);		


		getContentPane().setLayout(new GridLayout());
		setSize(240,110);
		//setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		setLocationByPlatform(true);

		setIconImage(VisualHelpers.getApplicationIconImage());
		
		final GridBagConstraints constr = new GridBagConstraints();
		constr.fill = GridBagConstraints.BOTH;
		constr.gridy = 0;
		constr.gridx = 0;
		constr.insets = new Insets(6, 6, 6, 6);
		//constr.weightx = 0.5; constr.weighty = 0.5;
		constr.weightx = 1; constr.weighty = 1;
		constr.anchor = GridBagConstraints.NORTHEAST;

		
		final JPanel panel = new JPanel();
		final JPanel borderPanel = new JPanel();
		//panel.setBorder(new EmptyBorder(5, 5, 5, 5));
		getContentPane().add(borderPanel);

		borderPanel.setLayout(new GridBagLayout());
		borderPanel.add(panel, constr);

		populateAboutPanel(panel);

		pack();

	}

	public static void populateAboutPanel(final JPanel panel) 
	{
		panel.setBorder(new EtchedBorder());
		panel.setLayout(new GridBagLayout());
		//panel.setBackground(Color.WHITE);
		panel.setBackground(VisualHelpers.getDigitalZoneLightColor());
		
		{
			final ImageIcon logo = new ImageIcon(  VisualHelpers.loadImage( "logo-dz.gif" ) ); //$NON-NLS-1$
			final JLabel label = new JLabel();
			final GridBagConstraints constr = new GridBagConstraints();
			constr.gridwidth = 1;
			constr.gridheight = 3;
			constr.fill = GridBagConstraints.NONE;
			constr.gridy = 0;
			constr.gridx = 3;
			constr.insets = new Insets(10, 0, 10, 10);
			//constr.weightx = 0.5; constr.weighty = 0.5;
			constr.weightx = 1; constr.weighty = 1;
			constr.anchor = GridBagConstraints.NORTHEAST;
			label.setIcon(logo);
			label.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
			panel.add(label, constr);

			label.setToolTipText(Messages.getString("AboutFrame.VisitSiteTooltip")); //$NON-NLS-1$
			label.addMouseListener(new MouseListener() {
				//@Override
				public void mouseClicked(MouseEvent e) {
					try {
						VisualHelpers.openUrl(new URI("http://dz.ru")); //$NON-NLS-1$
					} catch (IOException e1) {
					} catch (URISyntaxException e1) {
					}
				}

				//@Override
				public void mouseEntered(MouseEvent e) {				}

				//@Override
				public void mouseExited(MouseEvent e) {				}

				//@Override
				public void mousePressed(MouseEvent e) {			}

				//@Override
				public void mouseReleased(MouseEvent e) {				}

			});
		}

		{
			final JLabel label = new JLabel();
			label.setHorizontalAlignment(SwingConstants.CENTER);
			//label.setText("Digital Zone "+Version.NAME); //$NON-NLS-1$
			label.setText("Phantom Object Debugger"); //$NON-NLS-1$
			final GridBagConstraints constr = new GridBagConstraints();
			//constr.anchor = GridBagConstraints.WEST;
			//constr.ipadx = 35;
			constr.insets = new Insets(10, 10, 10, 10);
			constr.gridy = 0;
			constr.gridx = 2;
			constr.weightx = 1;
			constr.weighty = 1;
			panel.add(label, constr);
		}

		if(false){
			final JTextArea label = new JTextArea();

			label.setText(Messages.getString("AboutFrame.AboutText")); //$NON-NLS-1$
			label.setBackground(Color.WHITE);
			//label.setFont(getFont());
			//label.setHorizontalAlignment(SwingConstants.CENTER);
			//label.setBorder(new EtchedBorder(EtchedBorder.LOWERED));

			final GridBagConstraints constr = new GridBagConstraints();
			//constr.anchor = GridBagConstraints.WEST;
			//constr.ipadx = 35;
			constr.insets = new Insets(10, 10, 10, 10);
			constr.gridy = 1;
			constr.gridx = 2;
			constr.weightx = 1;
			constr.weighty = 1;
			//constr.ipadx = 4;
			//constr.ipady = 4;
			constr.fill = GridBagConstraints.HORIZONTAL;
			panel.add(label, constr);
		}




		{
			final JLabel label = new JLabel();
			label.setHorizontalAlignment(SwingConstants.CENTER);
			//label.setText(Messages.getString("AboutFrame.VersionMsg")+Version.VERSION);  //$NON-NLS-1$
			label.setText(Messages.getString("AboutFrame.VersionMsg")+"0.01");  //$NON-NLS-1$
			final GridBagConstraints constr = new GridBagConstraints();
			//constr.anchor = GridBagConstraints.WEST;
			//constr.ipadx = 35;
			//constr.insets = new Insets(0, 0, 0, 10);
			constr.insets = new Insets(10, 10, 10, 10);
			constr.gridy = 2;
			constr.gridx = 2;
			constr.weightx = 1;
			constr.weighty = 1;
			panel.add(label, constr);
		}
	}

	private static AboutFrame me; 
	public static void display() {
		if( me == null ) me = new AboutFrame();
		me.setVisible(true);
	}


	//protected void doQuit() { this.dispose(); }





}
