package ru.dz.pdb.ui.bits;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.font.FontRenderContext;
import java.awt.geom.Rectangle2D;

import javax.swing.JFrame;

public class HexView extends LinesView 
{
	private static final int N_CHARS_PER_LINE = 16;
	private static final int SPACE = 4;
	private final byte[] data;

	private Font myFont; 
	private int fontSize = 12;
	private Dimension d = new Dimension( N_CHARS_PER_LINE*32*2, 18 ); 
	
	public HexView(byte[] data) {
		this.data = data;

		//myFont = new Font(Font.MONOSPACED, Font.BOLD, fontSize);	
		myFont = new Font(Font.MONOSPACED, Font.PLAIN, fontSize);
		
		//setSize(getLineWidth(),getNLines()*getLineHeight());
		Dimension vd = new Dimension( getLineWidth() + SPACE*2, getNLines()*getLineHeight() + SPACE*2 );
		setSize(vd);
		setMinimumSize(vd);
		setPreferredSize(vd);
		//setVisible(true);
		//repaint();
	}

	
	private boolean sizeSet = false; 
	
	private void doSetSize(Graphics2D g2d) 
	{		
		String txt = byteArrayToHexString(data, 0*N_CHARS_PER_LINE, N_CHARS_PER_LINE);
		
		//Graphics2D g2d = (Graphics2D) getComponentGraphics(null);
		FontRenderContext frc = g2d.getFontRenderContext();
		Rectangle2D stringBounds = myFont.getStringBounds(txt, frc);
		
		d.height = (int) stringBounds.getHeight();
		d.width  = (int) stringBounds.getWidth();
		
		sizeSet = true;
	}
	
	
	@Override
	protected void paintComponent(Graphics g) {
		Graphics2D g2d = (Graphics2D) g;
		super.paintComponent(g);
		
		if(!sizeSet)
			doSetSize(g2d);
		
		g2d.setFont(myFont);
		
		g2d.setColor(Color.white);
		g2d.fillRect(0, 0, getWidth(), getHeight());

		g2d.setColor(Color.black);
		//g2d.drawString("safgjfg", 4, 10);
		
		int lineHeight = getLineHeight();
		//int lineWidth = getLineWidth();
		
		int n_lines = getNLines();
		
		for(int i = 0; i < n_lines; i++)
		{
			drawLine(i,g2d,lineHeight);
		}
	}



	private void drawLine(int i, Graphics2D g2d, int height) {
		String line = byteArrayToHexString(data, i*N_CHARS_PER_LINE, N_CHARS_PER_LINE);
		//System.out.println("HexView.drawLine(\""+line+"\")");
		g2d.drawString(line, SPACE, (i+1)*height + SPACE );
	}

	
	
	private int getNLines() {
		return 1+((data.length-1)/N_CHARS_PER_LINE);
	}


	private int getLineWidth() { return d.width; }
	
	public int getLineHeight() { return d.height; }



	/*
	static final String HEXCHARS[] = {
		"0", "1", "2", "3", "4", "5", "6", "7", 
		"8", "9", "A", "B", "C", "D", "E", "F"
	};
	*/

	/**
	 * Convert a byte[] array to hex dump.
	 * 
	 * @return result String buffer in String format 
	 * @param in byte[] buffer to convert to string format
	 */

	static String byteArrayToHexString( byte in[], int start, int len ) 
	{
		//byte ch;
		int i;
		int fill = len;

		if (in == null || in.length <= 0)
			return null;

		if(start+len > in.length)
			len = in.length-start;
		
		StringBuilder out = new StringBuilder(in.length * 4 + 10);

		out.append("| ");
		
		for(i = start; i < start+len; i++) 	
			out.append(String.format("%02X ", in[i]));		

		for(; i < start+fill; i++) 
			out.append("  ");

		out.append(" | ");

		for(i = start; i < start+len; i++) 
		{
			char c = (char)in[i];
			if( in[i] < 32) c = '.';
			out.append(c);
		}

		for(; i < start+fill; i++) 
			out.append(' ');
		
		out.append(" |");
		
		return out.toString();
	}    

	public static void main(String[] args) {
		JFrame f = new JFrame();
		
		//f.setLayout(new GridBagLayout());
		//f.setLayout(new BoxLayout(null,BoxLayout.X_AXIS));
		//f.setLayout(new 
		
		byte[] t = {0,1,2,0x4E};
		f.getContentPane().add(new HexView(t));
		f.pack();
		f.setVisible(true);
		f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
	}

}
