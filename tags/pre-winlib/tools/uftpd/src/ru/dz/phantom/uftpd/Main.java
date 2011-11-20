/**
 * 
 */
package ru.dz.phantom.uftpd;

/**
 * The dumbest ftpd ever. Very simple.
 */

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.Charset;

/**
 * @author dz
 *
 */
public class Main {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		System.out.println("uFTP Daemon");

		try {
			ServerSocket listen = new ServerSocket(21);
			
			System.out.println("Main.main() listen");
			Socket is = listen.accept();
			System.out.println("Main.main() accepted");
			serve( is );
			System.out.println("Main.main() finished serving");
			is.close();
			
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}

	static ServerSocket data;
	
	static String[] ignored = {
		"PASS",
		"SITE",
		"TYPE",
		"REST"
	};
	
	private static void serve(Socket ctl) throws IOException 
	{
		System.out.println("Main.serve() serving from "+ctl.getRemoteSocketAddress());
		
		//BufferedInputStream bctl = new BufferedInputStream(ctl.getInputStream());
		
		//InputStreamReader reader = new InputStreamReader(ctl.getInputStream());
		
		BufferedReader rctl = new BufferedReader(new InputStreamReader(ctl.getInputStream()));
		OutputStream octl = ctl.getOutputStream();

		say( octl, "220 Hi there\r\n" );
		
		outer: while(true)
		{
			String line = rctl.readLine();
			
			System.out.println("Main.serve() line='"+line+"'");
			
			String[] split = line.split("\\s");
			
			System.out.print("Main.serve() split=");
			for( String dmp : split)
			{
				System.out.print(" '"+dmp+"'");
			}
			System.out.println();
			
			if(split.length == 0)
			{
				sayln( octl, "500 Empty?");
				continue;
			}
			
			String cmd = split[0];
			
			System.out.println("Main.serve() cmd='"+cmd+"'");
			
			for( String ign : ignored)
			{
				if( cmd.equals(ign))
				{
					sayln( octl, "200 Successfully ignored" );
					continue outer;
				}
			}
			
			if( cmd.equals("QUIT"))
			{
				sayln( octl, "200 Bye" );
				System.exit(0);
			}

			if( cmd.equals("USER"))
			{
				sayln( octl, "230 Login successful." );
				continue;
			}

			if( cmd.equals("SYST"))
			{
				sayln( octl, "215 Unix" );
				continue;
			}

			if( cmd.equals("PWD"))
			{
				sayln( octl, "257 ./" );
				continue;
			}
			
			if( cmd.equals("XPWD"))
			{
				sayln( octl, "257 /" );
				continue;
			}

			if( cmd.equals("PASV"))
			{
				if(data != null) data.close();
				data = new ServerSocket(0);
				
				
				//SocketAddress da = data.getLocalSocketAddress();
				
				//InetAddress ia = data.getInetAddress();
				InetAddress ia = InetAddress.getLocalHost();  
				
				byte[] abytes = ia.getAddress();
				int aport = data.getLocalPort();

				
				
				sayln( octl, String.format("227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", 
						((int)abytes[0]) & 0xFF, 
						((int)abytes[1]) & 0xFF, 
						((int)abytes[2]) & 0xFF, 
						((int)abytes[3]) & 0xFF,
						((int)aport/256) & 0xFF, 
						((int)aport %256) & 0xFF 
						)
				);
				continue;
			}

			if( cmd.equals("LIST"))
			{
				String dir = ".";
				
				if(split.length > 1)
					dir = split[1];
				
				getList(octl, dir);
				continue;
			}

			if( cmd.equals("RETR"))
			{
				sendFile( octl, split[1] );
				continue;
			}

			
			System.out.println("Main.serve() unknown cmd='"+cmd+"'");
			sayln( octl, "500 What?");
			
		}
		
		
	}

	private static void sendFile(OutputStream octl, String name) throws IOException {
		System.out.println("sendFile()");
		
		Socket ds = data.accept();
		System.out.println("sendFile() accepted");
		
		OutputStream out = ds.getOutputStream();
		
		InputStream in;
		try {
			in = new FileInputStream(name);
		}
		catch(FileNotFoundException e) { 
			sayln( octl, "500 No file");
			return;
		}
		 
		
		sayln( octl, "150 OK");

		byte[] buf = new byte[1024]; 
		int len; 
		while ((len = in.read(buf)) > 0) 
		{ 
			out.write(buf, 0, len); 
		} 

		
		in.close(); 
		out.close(); 		

		sayln( octl, "226 Done");
		
		System.out.println("sendFile() sent");
		ds.close();
		System.out.println("sendFile() closed");
	}

	private static void sendData(String datas) throws IOException {
		System.out.println("Main.sendData()");
		byte[] bytes = datas.getBytes(Charset.forName("ASCII"));
		Socket ds = data.accept();
		System.out.println("Main.sendData() accepted");
		ds.getOutputStream().write(bytes);		
		System.out.println("Main.sendData() sent");
		ds.close();
		System.out.println("Main.sendData() closed");
	}

	private static void getList(OutputStream octl, String dirName) throws IOException {
		
		File dir = new File(dirName); 
		String[] children = dir.list(); 
		if (children == null) 
		{ 
			// Either dir does not exist or is not a directory
			sayln( octl, "500 No dir");
			return;
		} 
		else 
		{ 
			sayln( octl, "150 OK");

			StringBuilder list = new StringBuilder();
			
			for (int i=0; i<children.length; i++) 
			{ 
				// Get filename of file or directory 
				String filename = children[i];
				File child = new File(filename);
				
				list.append( child.isDirectory() ? 'd' : '-' );
				list.append( child.canRead() ? 'r' : '-' );
				list.append( child.canWrite() ? 'w' : '-' );
				list.append( child.canExecute() ? 'x' : '-' );
				list.append( "------+ 1 none none "+child.length()+" " );
				list.append( "2010-01-01 00:00 ");
				
				list.append( child.getName() );
				list.append( "\r\n");
			} 

			sendData( list.toString() );
			
			sayln( octl, "226 Done");
		} 
		
		
		
	}

	private static void say(OutputStream octl, String string) throws IOException {
		System.out.println("Main.say("+string+")");
		byte[] bytes = string.getBytes(Charset.forName("ASCII"));
		octl.write(bytes);		
	}

	private static void sayln(OutputStream octl, String string) throws IOException {
		say(octl, string+"\r\n");
	}
}
