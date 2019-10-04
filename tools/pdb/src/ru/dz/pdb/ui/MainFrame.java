package ru.dz.pdb.ui;

import java.awt.Container;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;

import javax.swing.AbstractAction;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTabbedPane;
import javax.swing.KeyStroke;

import ru.dz.pdb.Main;
import ru.dz.pdb.misc.AboutFrame;
import ru.dz.pdb.misc.VisualHelpers;


public class MainFrame extends JFrame 
{
	protected ThreadListPanel tlPanel = new ThreadListPanel();
	protected ClassListPanel clPanel = new ClassListPanel();

	public MainFrame() {
		setTitle( "Phantom OS object debugger");

		setLocationByPlatform(true);
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		populateMe();
		setJMenuBar(makeMenu());

		setAlwaysOnTop(true);

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

		gbc.fill = GridBagConstraints.HORIZONTAL;

		JPanel topPanel = new JPanel(new GridBagLayout());
		contentPane.add(topPanel, gbc);
		poulateTopPanel(topPanel);

		gbc.fill = GridBagConstraints.BOTH;
		gbc.weightx = 1;
		gbc.weighty = 1;
		JPanel mainPanel = new JPanel(new GridBagLayout());
		contentPane.add(mainPanel, gbc);
		poulateMainPanel(mainPanel);

	}

	private void poulateMainPanel(JPanel panel) {
		GridBagConstraints gbc = new GridBagConstraints();
		gbc.weightx = gbc.weighty = 1;

		gbc.gridx = GridBagConstraints.RELATIVE;
		gbc.gridy = 0;	

		gbc.anchor = GridBagConstraints.NORTHWEST;

		gbc.fill = GridBagConstraints.BOTH;

		Dimension pref = new Dimension(450, 200);

		JTabbedPane tabs = new JTabbedPane();

		{
			//tlPanel = new ThreadListPanel();
			JScrollPane scroll = new JScrollPane(tlPanel);
			scroll.setPreferredSize(pref);
			scroll.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);

			tabs.addTab("Threads", scroll);
		}

		{
			JScrollPane scroll = new JScrollPane(clPanel);
			scroll.setPreferredSize(pref);
			scroll.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);

			tabs.addTab("Classes", scroll);
		}

		//panel.add( scroll, gbc );
		panel.add( tabs, gbc );
	}

	private void poulateTopPanel(JPanel panel) {
		GridBagConstraints gbc = new GridBagConstraints();
		gbc.weightx = gbc.weighty = 1;

		gbc.gridx = GridBagConstraints.RELATIVE;
		gbc.gridy = 0;	

		gbc.anchor = GridBagConstraints.NORTHWEST;

		//panel.add( new JLabel("Inspect:"), gbc );

		{
			JButton iRoot = new JButton(new AbstractAction("Root") {		
				@Override
				public void actionPerformed(ActionEvent e) {
					Main.inspectRootObject();

				}
			});
			iRoot.setIcon(VisualHelpers.loadIcon("root.png"));
			panel.add( iRoot, gbc );
		}
		{
			JButton b = new JButton(new AbstractAction("Reload") {		
				@Override
				public void actionPerformed(ActionEvent e) {
					reloadContent();

				}
			});
			b.setIcon(VisualHelpers.loadIcon("download.png"));
			panel.add( b, gbc );
		}
		//panel.add( new RefButton(parsed.getClassRef(),"Class"), gbc );
		//panel.add( new RefButton(parsed.getObjectSatellites(),"Sat"), gbc );

	}


	protected void reloadContent() {
		tlPanel.reload();
		clPanel.reload();

	}

	private JMenuBar makeMenu()
	{
		// --------------------------------------------------------------------
		// Menu
		// --------------------------------------------------------------------

		JMenuBar menuBar = new JMenuBar();
		//this.setJMenuBar(menuBar);

		{
			JMenu fileMenu = new JMenu("File"); //$NON-NLS-1$
			fileMenu.setMnemonic(KeyEvent.VK_F);
			menuBar.add(fileMenu);

			{
				JMenuItem item = new JMenuItem("Open project");
				item.setMnemonic(KeyEvent.VK_O);
				item.setAccelerator(KeyStroke.getKeyStroke(
						KeyEvent.VK_O, ActionEvent.ALT_MASK));
				item.setToolTipText("Load project from file");
				fileMenu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.openProject(); }
				});
			}

			{
				JMenuItem item = new JMenuItem("Save project");
				item.setMnemonic(KeyEvent.VK_S);
				item.setAccelerator(KeyStroke.getKeyStroke(
						KeyEvent.VK_S, ActionEvent.ALT_MASK));
				item.setToolTipText("Save project to file");
				fileMenu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.saveProject(); }
				});
			}

			{
				JMenuItem item = new JMenuItem("Save project as");
				//item.setMnemonic(KeyEvent.VK_A);
				//item.setAccelerator(KeyStroke.getKeyStroke(	KeyEvent.VK_A, ActionEvent.ALT_MASK));
				item.setToolTipText("Save project to file");
				fileMenu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.saveProjectAs(); }
				});
			}

			{
				JMenuItem item = new JMenuItem("Edit project");
				//item.setMnemonic(KeyEvent.VK_R);
				//item.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_R, ActionEvent.ALT_MASK));
				//item.setToolTipText("Reconnect to host");
				fileMenu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.editProject(); }
				});
			}

			fileMenu.addSeparator();

			{
				JMenuItem item = new JMenuItem("Reconnect");
				//item.setMnemonic(KeyEvent.VK_R);
				//item.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_R, ActionEvent.ALT_MASK));
				item.setToolTipText("Reconnect to host");
				fileMenu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.getHc().reconnect();; }
				});
			}

			fileMenu.addSeparator();
			
			{
				JMenuItem item = new JMenuItem("Disassemble");
				//item.setMnemonic(KeyEvent.VK_R);
				//item.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_R, ActionEvent.ALT_MASK));
				item.setToolTipText("Disassemble bytecode (.pc) file");
				fileMenu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.disassemble(); }
				});
			}

			fileMenu.addSeparator();

			{
				JMenuItem itemQuit = new JMenuItem("Quit"); //$NON-NLS-1$
				//itemQuit.setMnemonic(KeyEvent.VK_Q);
				itemQuit.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F4, ActionEvent.ALT_MASK));
				//itemQuit.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Q, 0/*ActionEvent.ALT_MASK*/));

				itemQuit.setToolTipText("Quit debugger"); //$NON-NLS-1$
				fileMenu.add(itemQuit);
				itemQuit.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.doQuit(); }
				});
			}			

		}



		{
			JMenu menu = new JMenu("Debug"); //$NON-NLS-1$
			menu.setMnemonic(KeyEvent.VK_D);
			//menu.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_D, ActionEvent.ALT_MASK));
			menuBar.add(menu);

			{
				JMenuItem item = new JMenuItem("Run last"); //$NON-NLS-1$
				item.setToolTipText("Run last class"); //$NON-NLS-1$
				//menu.setMnemonic(KeyEvent.VK_R);
				//menu.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_R, ActionEvent.ALT_MASK));
				menu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.runLastClass(); }
				});
			}

			{
				JMenuItem item = new JMenuItem("Run class"); //$NON-NLS-1$
				item.setToolTipText("Run class"); //$NON-NLS-1$
				//menu.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_R, ActionEvent.ALT_MASK|ActionEvent.CTRL_MASK));
				menu.setMnemonic(KeyEvent.VK_C);
				menu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.runClass(); }
				});
			}

			menu.addSeparator();

			{
				JMenuItem item = new JMenuItem("Inspect"); //$NON-NLS-1$
				item.setMnemonic(KeyEvent.VK_I);
				item.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_I, ActionEvent.ALT_MASK));

				item.setToolTipText("Inspect object by address"); //$NON-NLS-1$
				menu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.inspect(); }
				});
			}			

			{
				JMenuItem item = new JMenuItem("Inspect Root"); //$NON-NLS-1$
				//itemQuit.setMnemonic(KeyEvent.VK_Q);
				//item.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_R, ActionEvent.ALT_MASK));
				//itemQuit.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Q, 0/*ActionEvent.ALT_MASK*/));
				item.setMnemonic(KeyEvent.VK_R);
				item.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_R, ActionEvent.ALT_MASK));

				item.setToolTipText("Inspect root object"); //$NON-NLS-1$
				menu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { Main.inspectRootObject(); }
				});
			}			

		}






		{
			JMenu helpMenu = new JMenu("Help"); //$NON-NLS-1$
			helpMenu.setMnemonic(KeyEvent.VK_F1);
			menuBar.add(helpMenu);

			{
				JMenuItem item = new JMenuItem("About"); //$NON-NLS-1$
				item.setToolTipText("Open about dialog"); //$NON-NLS-1$
				helpMenu.add(item);
				item.addActionListener(new ActionListener() {
					public void actionPerformed(ActionEvent e) { AboutFrame.display(); }
				});
			}
		}
		return menuBar;
	}


}
