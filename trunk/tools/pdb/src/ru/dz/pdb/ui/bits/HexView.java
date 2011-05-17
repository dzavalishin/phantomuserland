package ru.dz.pdb.ui.bits;

import java.awt.Color;
import java.awt.Component;
import java.awt.Graphics;
import java.awt.Graphics2D;

import javax.swing.JComponent;
import javax.swing.JFrame;

public class HexView extends JComponent 
{
	private static final int N_CHARS_PER_LINE = 16;
	private final byte[] data;

	public HexView(byte[] data) {
		this.data = data;
		setSize(getLineWidth(),getNLines()*getLineHeight());
		//setVisible(true);
		repaint();
	}

	@Override
	protected void paintComponent(Graphics g) {
		Graphics2D g2d = (Graphics2D) g;
		super.paintComponent(g);
		
		g.setColor(Color.lightGray);
		g.fillRect(0, 0, getWidth(), getHeight());
		
		int lineHeight = getLineHeight();
		int lineWidth = getLineWidth();
		
		g.setColor(Color.black);
		int n_lines = getNLines();
		
		for(int i = 0; i < n_lines; i++)
		{
			drawLine(i,g2d,lineHeight);
		}
	}




	private int getNLines() {
		return 1+((data.length-1)/N_CHARS_PER_LINE);
	}

	private void drawLine(int i, Graphics2D g2d, int height) {
		String line = byteArrayToHexString(data, i*N_CHARS_PER_LINE, N_CHARS_PER_LINE);
		System.out.println("HexView.drawLine(\""+line+"\")");
		g2d.drawString(line, 4, i*height);
	}

	private int getLineWidth() {
		return N_CHARS_PER_LINE*32;
	}


	private int getLineHeight() {
		return 20;		
	}



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
		byte ch;
		int i; 

		if (in == null || in.length <= 0)
			return null;

		if(start+len > in.length)
			len = in.length-start;
		
		StringBuilder out = new StringBuilder(in.length * 4 + 2);

		for(i = start; i < start+len; i++) 
		{
			out.append(String.format("%02X", in[i]));
		}

		out.append(' ');

		for(i = start; i < start+len; i++) 
		{
			char c = (char)in[i];
			if( in[i] < 32) c = '.';
			out.append(c);
		}

		return out.toString();
	}    

	public static void main(String[] args) {
		JFrame f = new JFrame();
		
		byte[] t = {0,1,2,0x4E};
		f.add(new HexView(t));
		f.pack();
		f.setVisible(true);
		f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
	}

}
