package ru.dz.pdb.misc;

import java.awt.Color;
import java.awt.Desktop;
import java.awt.Frame;
import java.awt.Image;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.imageio.ImageIO;
import javax.swing.ImageIcon;
import javax.swing.JOptionPane;
import javax.swing.JPanel;


public abstract class VisualHelpers {
    private static final Logger log = Logger.getLogger(VisualHelpers.class.getName());

	static public BufferedImage loadImage(String shortName)
	{
		InputStream in = VisualHelpers.class.getClassLoader().getResourceAsStream(shortName);
		if(in == null) { return null; }
		try {
			return ImageIO.read(in);
		} catch (IOException e) {
			log.log(Level.SEVERE,"Image read error", e); //$NON-NLS-1$
			return null;
		}		
	}

	static public ImageIcon loadIcon(String shortName)
	{
		final BufferedImage image = loadImage(shortName);
		if(image == null) return null;
		return new ImageIcon(image);
	}
	
	static final Color dzLight = new Color(249,219,50);
	public static Color getDigitalZoneLightColor() { return dzLight; }
	
	static final Color dzMedium = new Color(241,171,40);
	public static Color getDigitalZoneMediumColor() { return dzMedium; }
	
	static final Color dzDark = new Color(234,140,32);
	public static Color getDigitalZoneDarkColor() { return dzDark; }
	
	static public void showMessageDialog(JPanel referencePanel, String text)
	{
		Frame frame= JOptionPane.getFrameForComponent(referencePanel);
	    JOptionPane.showMessageDialog(frame, text);		
	}


	private static BufferedImage appIconImage;
	/**
	 * @return Application icon image.
	 */
	public static BufferedImage getApplicationIconImage() {
		if(appIconImage == null)
		{
			appIconImage = loadImage("icon.png"); //$NON-NLS-1$
		}
		return appIconImage;
	}

	
	private static Desktop desktop = null;
	
	/**
	 * Opens system web browser and directs it to given URI.
	 * @param uri
	 * @throws IOException
	 */
	public static void openUrl(URI uri) throws IOException
	{
	    // Before more Desktop API is used, first check 
	    // whether the API is supported by this particular 
	    // virtual machine (VM) on this particular host.
	    if(desktop == null && Desktop.isDesktopSupported()) 
	        desktop = Desktop.getDesktop();
	    
	    if(desktop == null) return;
	    
	    desktop.browse(uri);	    
	}
	
}


