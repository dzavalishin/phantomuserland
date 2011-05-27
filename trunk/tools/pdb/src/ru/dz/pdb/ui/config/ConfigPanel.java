package ru.dz.pdb.ui.config;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.io.Serializable;

import javax.swing.Icon;
import javax.swing.JPanel;

public abstract class ConfigPanel extends JPanel implements Serializable {
	
	private static final long serialVersionUID = 1665003886963911462L;
	protected ConfigPanel()	{		super(new GridBagLayout());	}

	protected final GridBagConstraints cons;


		{
			GridBagConstraints c = new GridBagConstraints();
			c.weightx = 1;
			c.weighty = 1;
			c.gridx = 0;
			c.gridy = GridBagConstraints.RELATIVE;
			c.anchor = GridBagConstraints.NORTHEAST;
			c.ipadx = 4;
			c.ipady = 2;
			c.insets = new Insets(4,4,4,4);
			c.fill = GridBagConstraints.BOTH;
			cons = c;
		}
	
	
	public abstract String getName(); // { return "undefined tab"; }
	public abstract String getTip(); // { return "undefined tab"; }
	public Icon getIcon() {		return null;	}

	public abstract void discardChanges();
	public abstract void applyChanges();

}
