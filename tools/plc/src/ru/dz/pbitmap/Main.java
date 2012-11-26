package ru.dz.pbitmap;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

import javax.imageio.ImageIO;

public class Main {
	static boolean isBGRA = true;
	/**
	 * @param args
	 * @throws IOException 
	 */
	public static void main(String[] args) throws IOException {
		if(args.length != 1)
		{
			System.out.println(
					"Convert PNG file to Phantom bitmap C source file.\n" +
					"Usage: pbitmap infile.png"
					);
			return;
		}

		File inFn = new File(args[0]);		

		System.out.println("Converting "+inFn);
		
		File outFn = new File(inFn.getCanonicalFile()+".c");		
		FileWriter fw = new FileWriter(outFn);
		
		String varBase = inFn.getName().replaceAll("\\..*", "");
		
		fw.write("// Bitmap "+varBase+"\n");
		fw.write("// Automatically created from "+inFn+"\n\n");
		
		BufferedImage image = ImageIO.read(inFn);
		
		if(image == null)
		{
			System.out.println("Can't read image data for "+inFn);
			fw.close();
			return;
		}
		int w = image.getWidth();
	    int h = image.getHeight();
		
		fw.write("#include <video/screen.h>\n\n");
		
		fw.write("drv_video_bitmap_t "+varBase+"_bmp = {" + w +", "+ h +", { \n");
		//fw.write("char "+varBase+"_data[] = {\n");
		
	    System.out.println("width, height: " + w + ", " + h);

	    for (int i = h-1; i >=0; i--) {
	      for (int j = 0; j < w; j++) {
	        
	        int pixel = image.getRGB(j, i);
	        
	        if(isBGRA)
	        	printPixelBGRA(fw, pixel);
	        else
	        	printPixelRGBA(fw, pixel);
	      }
	      fw.write("\n");
	    }
		
	    fw.write("}\n};\n\n");
	    
		fw.close();
	}

	private static void printPixelRGBA(FileWriter fw, int pixel) throws IOException {
	    int alpha = (pixel >> 24) & 0xff;
	    int red = (pixel >> 16) & 0xff;
	    int green = (pixel >> 8) & 0xff;
	    int blue = (pixel) & 0xff;
		
	    fw.write(String.format("{0x%X, 0x%x, 0x%x, 0x%X}, ", red, green, blue, alpha));
	}

	private static void printPixelBGRA(FileWriter fw, int pixel) throws IOException {
	    int alpha = (pixel >> 24) & 0xff;
	    int red = (pixel >> 16) & 0xff;
	    int green = (pixel >> 8) & 0xff;
	    int blue = (pixel) & 0xff;
		
	    fw.write(String.format("{ 0x%X, 0x%x, 0x%x, 0x%X}, ", blue, green, red, alpha));
	}
	
	
}
