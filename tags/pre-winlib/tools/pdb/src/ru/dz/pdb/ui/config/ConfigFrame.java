package ru.dz.pdb.ui.config;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.util.LinkedList;
import java.util.List;
import java.util.logging.Logger;

import javax.swing.AbstractAction;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JTabbedPane;
import javax.swing.border.LineBorder;

import ru.dz.pdb.Main;
import ru.dz.pdb.misc.VisualHelpers;


/**
 * NB! This class must be instantiated AFTER the configuration was loaded!
 * @author dz
 *
 */

public class ConfigFrame extends JFrame {
	/**
	 * 
	 */
	private static final long serialVersionUID = -5362992357427104628L;

	private static final Logger log = Logger.getLogger(ConfigFrame.class.getName()); 

	//private Configuration c = ConfigurationFactory.getConfiguration();
	
	private ConfigPanelGeneral configPanelGeneral = new ConfigPanelGeneral(Main.getProject());
	//private ConfigPanelColors configPanelColors = new ConfigPanelColors();

	//private ConfigPanelList<CliUser> configPanelUsers = new ConfigPanelList<CliUser>(Messages.getString("ConfigFrame.UsersTab"),Messages.getString("ConfigFrame.UsersTabTip"),CliUser.class, c.getUserItems(), c.getUserLibItems()); //$NON-NLS-1$ //$NON-NLS-2$
	
	private JTabbedPane tabbedPane = new JTabbedPane(JTabbedPane.TOP,JTabbedPane.WRAP_TAB_LAYOUT);

	private JPanel globalPanel = new JPanel();

	private JButton saveButton, saveVisButton;


	public ConfigFrame() {
		super();
		initialize();
	}

	private void initialize() {
        this.setDefaultCloseOperation(JFrame.HIDE_ON_CLOSE);
		//this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        this.setMinimumSize(new Dimension(700, 300));
        this.setTitle("Project"); //$NON-NLS-1$
        this.setIconImage(VisualHelpers.getApplicationIconImage());
        this.setLocationByPlatform(true);
        

        GridBagConstraints c = new GridBagConstraints();
        c.weightx = c.weighty = 1;
        c.fill = GridBagConstraints.BOTH;
        c.gridx = c.gridy = 0;
        c.insets = new Insets(1,4,4,4);
        
        getContentPane().setLayout(new GridBagLayout());
        getContentPane().add(tabbedPane, c);
        
        c.gridy = 1;
        c.fill = GridBagConstraints.HORIZONTAL;
        c.weighty = 0;
        getContentPane().add(globalPanel, c);
        fillGlobalPanel(globalPanel);

        addTab(configPanelGeneral);
        //addTab(configPanelColors);
        
		//addTab(configPanelUsers);
		
        this.pack();        
        //this.setVisible(true);        
	}

	@SuppressWarnings("serial")
	private void fillGlobalPanel(JPanel gp) {

		gp.setBorder(new LineBorder(Color.GRAY, 1));
		gp.setBackground(Color.decode("#999999"));
		
		
		saveButton = new JButton();
		gp.add(saveButton);
		saveButton.setAction(new AbstractAction() {

			public void actionPerformed(ActionEvent e) {
				applyChanges();
				Main.saveProject();				
			}});
		saveButton.setText("Save"); //$NON-NLS-1$
		//saveButton.setToolTipText(Messages.getString("ConfigFrame.SaveSetupButtonDescription")); //$NON-NLS-1$
        saveButton.setIcon(VisualHelpers.loadIcon("settings_save.png"));

		
		JButton applyButton = new JButton();
		//panel.add(new JLabel("Apply: "), consL);
		applyButton.addActionListener(new AbstractAction() { public void actionPerformed(ActionEvent e) { applyChanges(); }});
		applyButton.setText("Apply");		 //$NON-NLS-1$
		gp.add(applyButton);

		JButton discardButton = new JButton();
		//panel.add(new JLabel("System stop: "), consL);
		discardButton.addActionListener(new AbstractAction() { public void actionPerformed(ActionEvent e) { discardChanges(); }});
		discardButton.setText("Discard");		 //$NON-NLS-1$
		gp.add(discardButton);
		
        applyButton.setIcon(VisualHelpers.loadIcon("settings_apply.png"));
        discardButton.setIcon(VisualHelpers.loadIcon("settings_discard.png"));
		
		
	}

	private List<ConfigPanel> tlist = new LinkedList<ConfigPanel>();
	
	private void addTab(ConfigPanel p) {
		tabbedPane.addTab(p.getName(), p.getIcon(), p, p.getTip());
		tlist.add(p);
	}
	
	protected void discardChanges() {
		for( ConfigPanel p : tlist )
			p.discardChanges();
		repaint(100);
	}

	protected void applyChanges() {
		for( ConfigPanel p : tlist )
			p.applyChanges();		
	}




	public void enableConfigTabs(boolean enabled)
	{
/*		
		saveButton.setEnabled(enabled);
		
		if(!enabled)
			tabbedPane.setSelectedComponent(configPanelGeneral);

		for( int i = 0; i < tabbedPane.getComponentCount(); i++ )
		{
			if( tabbedPane.getComponentAt(i) == configPanelGeneral )
			{
				tabbedPane.setEnabledAt(i, true);
			}
			else
				tabbedPane.setEnabledAt(i, enabled);
		}
*/		
	}


    
}
