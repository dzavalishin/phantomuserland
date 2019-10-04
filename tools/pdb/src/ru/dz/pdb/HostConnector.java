package ru.dz.pdb;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.nio.charset.Charset;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

public class HostConnector {
	private static final Logger log = Logger.getLogger(HostConnector.class.getName()); 

	private static final int BUFMAX = 20480;
	private static final int PVM_ALLOC_HDR_SIZE = 16;
	private Socket s;

	private InetAddress host;

	private boolean connected = false;

	private static Object commSema = new Object(); 

	public HostConnector() throws UnknownHostException, IOException {
		host = InetAddress.getByName("127.0.0.1");
		//connect();
	}

	public void connect() throws IOException {
		s = new Socket( host , 1256);
		s.setSoTimeout(300);
		connected = true;
	}

	private void putDebugChar(byte c) throws IOException
	{
		s.getOutputStream().write(c);
	}

	private byte getDebugChar() throws IOException
	{
		byte c = (byte)s.getInputStream().read();
		return c;
	}



	public void disconnect() {
		/*
		try {
			s.shutdownInput();
			s.shutdownOutput();
			s.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		 */		
		connected = false;
	}

	// Something is wrong with it - doesnt work, kills communications
	public void reconnect() {
		//synchronized (commSema) 
		{
			try {

				//s.shutdownInput();
				//s.shutdownOutput();
				s.close();

				connect();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}

	}	


	// --------------------------------------------------------------------
	// Packets
	// --------------------------------------------------------------------



	/*
	 * send the packet in buffer.
	 */
	private void putpacket(String buffer) throws IOException, CmdException
	{
		int checksum;
		int count;
		char ch;
		int tries = 10;

		int blen = buffer.length();

		/*
		 * $<packet info>#<checksum>.
		 */

		while(true)
		{
			if( tries-- <= 0 )
				throw new CmdException("No ack");

			log.log(Level.FINEST,"HostConnector.putpacket("+buffer+")");
			putDebugChar((byte) '$');
			checksum = 0;
			count = 0;

			while(count < blen) 
			{
				ch = buffer.charAt(count);
				putDebugChar((byte)ch);
				checksum += 0xFF & ((int)ch);
				count++;
			}

			putDebugChar((byte) '#');
			checksum &= 0xFF;
			putDebugChar(hexchars(checksum >> 4));
			putDebugChar(hexchars(checksum & 0xf));

			//putDebugChar((byte) '\n');

			try {
				byte c = (byte) (getDebugChar() & 0x7f);
				if( c == '+' )
				{
					log.log(Level.FINEST,"HostConnector.putpacket() got ACK");
					return;
				}
				else if( c == '-' )
				{
					log.log(Level.FINEST,"HostConnector.putpacket() got NAK");
				}
				else 
				{
					log.log(Level.FINEST,"HostConnector.putpacket() got not +/- but '"+(char)c+"'");
				}
			} catch (SocketTimeoutException e)
			{
				continue;
			}
		}

	}




	/*
	 * scan for the sequence $<data>#<checksum>
	 */
	private String getpacket() throws ChecksumException, IOException
	{
		StringBuilder buffer = new StringBuilder(128);
		int checksum;
		int xmitcsum;
		//int i;
		int count;
		char ch;

		/*
		 * wait around for the start character,
		 * ignore all other characters
		 */
		while((ch = (char) (getDebugChar() & 0x7f)) != '$') 
			;

		log.log(Level.FINEST,"HostConnector.getpacket() $");

		checksum = 0;
		xmitcsum = (char) -1;
		count = 0;

		/*
		 * now, read until a # or end of buffer is found
		 */
		while (count < BUFMAX) {
			ch = (char) (getDebugChar() & 0x7f);
			if (ch == '#')
				break;
			checksum = (byte) (checksum + ch);
			buffer.append( ch );
			count = count + 1;
		}

		if (count >= BUFMAX)
			throw new ChecksumException();

		if (ch == '#') {
			xmitcsum = hex((char) (getDebugChar() & 0x7f)) << 4;
			xmitcsum |= hex((char) (getDebugChar() & 0x7f));

			checksum &= 0xFF;

			if (checksum != xmitcsum)
			{
				log.log(Level.WARNING,String.format("HostConnector.getpacket() my csum %x his %x ", checksum , xmitcsum));
				putDebugChar((byte) '-');
				throw new ChecksumException();
			}

		}

		putDebugChar((byte) '+');    
		return buffer.toString();
	}





	// --------------------------------------------------------------------
	// hex
	// --------------------------------------------------------------------



	private byte hexchars(int i) {
		if( i < 10 )
			return (byte) ('0'+i);
		return (byte) ('a'+i-10);
	}

	/*
	 * Convert ch from a hex digit to an int
	 */
	static int hex(char ch)
	{
		if (ch >= 'a' && ch <= 'f')
			return ch-'a'+10;
		if (ch >= '0' && ch <= '9')
			return ch-'0';
		if (ch >= 'A' && ch <= 'F')
			return ch-'A'+10;
		return -1;
	}





	/**
	 * convert the hex array buf into binary to be placed in mem
	 * return a pointer to the character AFTER the last byte written
	 */
	static void hex2mem(String buf, byte[] mem, int count )
	{
		int i;
		byte ch;

		for( i=0; i<count; i++ )
		{
			ch = (byte) (hex(buf.charAt(i*2)) << 4);
			ch |= hex(buf.charAt(i*2+1));
			mem[i] = ch;
		}
	}



	// --------------------------------------------------------------------
	// Generic cmd processing
	// --------------------------------------------------------------------



	private String execCmd( String req ) throws IOException, ChecksumException, CmdException
	{
		int tries = 5;
		while(tries-- > 0)
		{
			try {
				synchronized (commSema) {
					putpacket(req);
					return getpacket();
				}
			} catch (ChecksumException e) {
				log.log(Level.SEVERE,"Packet checksum error");
			}
		}

		throw new ChecksumException();
	}

	private void execOkCmd(String cmd) {
		try {
			String reply = execCmd(cmd);
			if( !reply.equals("OK") )
				log.log(Level.SEVERE,"HostConnector.execOkCmd() - NOT OK: "+reply);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (ChecksumException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (CmdException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}		
	}



	private int intAt(byte[] ah, int pos) {
		int i;

		i =  (((int)ah[pos+0]) & 0xFF);
		i |= (((int)ah[pos+1]) & 0xFF) << 8;
		i |= (((int)ah[pos+2]) & 0xFF) << 16;
		i |= (((int)ah[pos+3]) & 0xFF) << 24;

		return i;
	}


	// --------------------------------------------------------------------
	// Commands
	// --------------------------------------------------------------------



	/*
	 * mAA..AA,LLLL  Read LLLL bytes at address AA..AA
	 */
	public byte[] cmdGetMem(long address, int size) throws CmdException
	{
		String cmd = String.format("m%x,%x", address, size );
		try {

			String reply = execCmd(cmd);
			byte [] result = new byte[size];

			hex2mem(reply, result, size);
			return result;

		} catch (IOException e) {
			throw new CmdException("IO error", e);
		} catch (ChecksumException e) {
			throw new CmdException("Checksum error", e);
		}
	}

	public byte[] cmdGetObject(long address) throws CmdException
	{
		log.log(Level.FINE,String.format("cmdGetObject @%X", address));

		byte[] ah = cmdGetMem( address, PVM_ALLOC_HDR_SIZE);

		int oSize = intAt(ah,3*4);
		log.log(Level.FINE,String.format("osize %X", oSize));

		byte[] o = cmdGetMem( address, oSize);
		return o;
	}

	// Get persistent pool start address 
	public long cmdGetPoolAddress() throws CmdException
	{
		String cmd = String.format(":p" );
		try {

			String reply = execCmd(cmd);
			return Long.parseLong(reply, 16);
		} catch (IOException e) {
			throw new CmdException("IO error", e);
		} catch (ChecksumException e) {
			throw new CmdException("Checksum error", e);
		}
	}

	public void cmdRunClass(String runClassName, int runClassMethod) {
		String cmd = String.format(":r%x,%s", runClassMethod, runClassName );
		execOkCmd(cmd);
	}

	// --------------------------------------------------------------------
	// Threads
	// --------------------------------------------------------------------


	public boolean fThreadInfo(List<Integer> ret) throws CmdException {
		return doThreadInfo("qfThreadInfo", ret);
	}

	public boolean sThreadInfo(List<Integer> ret) throws CmdException {
		return doThreadInfo("qsThreadInfo", ret);
	}

	private boolean doThreadInfo(String cmd, List<Integer> ret) throws CmdException {
		try {
			String reply = execCmd(cmd);
			if(reply.equalsIgnoreCase("l"))
				return false;

			if(!reply.startsWith("m "))
				throw new CmdException("ThreadInfo syntax: "+reply);

			reply = reply.substring(2); // skip "m "

			String[] tids = reply.split(",");
			//System.out.println("HostConnector.doThreadInfo("+cmd+") = '"+reply+"'");
			for( String ts : tids )
			{
				int radix = 10;
				if(ts.startsWith("0x")||ts.startsWith("0X"))
				{
					ts = ts.substring(2);
					radix = 16;
				}

				int tid = Integer.parseInt(ts, radix);
				ret.add(tid);
			}

			return true;

		} catch (IOException e) {
			throw new CmdException("IO error", e);
		} catch (ChecksumException e) {
			throw new CmdException("Checksum error", e);
		}

		//return false;
	}	


	public String cmdThreadExtraInfo(int tid) throws CmdException {
		try {
			String reply = execCmd("qThreadExtraInfo,"+tid);

			if(reply.startsWith("E"))
				throw new CmdException("Error: "+reply);

			byte[] mem = new byte[reply.length()/2];
			hex2mem(reply, mem, mem.length);

			return new String(mem,Charset.forName("UTF-8"));

		} catch (IOException e) {
			throw new CmdException("IO error", e);
		} catch (ChecksumException e) {
			throw new CmdException("Checksum error", e);
		}

	}

	public boolean isConnected() {
		return connected ;
	}




}
