package ru.dz.pdb;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

public class HostConnector {
	private static final int BUFMAX = 20480;
	private Socket s;
	
	public HostConnector() throws UnknownHostException, IOException {
		s = new Socket(InetAddress.getByName("127.0.0.1") , 1256);
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
	
	/*
	 * send the packet in buffer.
	 */
	private void putpacket(String buffer) throws IOException
	{
	    int checksum;
	    int count;
	    char ch;

	    int blen = buffer.length();
	    
	    /*
	     * $<packet info>#<checksum>.
	     */

	    do {
	        putDebugChar((byte) '$');
	        checksum = 0;
	        count = 0;
	        
	        while(count < blen) 
	        {
	        	ch = buffer.charAt(count);
	            putDebugChar((byte)ch);
	            checksum += ch;
	            count++;
	        }

	        putDebugChar((byte) '#');
	        putDebugChar(hexchars(checksum >> 4));
	        putDebugChar(hexchars(checksum & 0xf));

	    }
	    while ((getDebugChar() & 0x7f) != '+');
	}
	

	
	
	/*
	 * scan for the sequence $<data>#<checksum>
	 */
	private String getpacket() throws ChecksumException, IOException
	{
		StringBuilder buffer = new StringBuilder(128);
	    int checksum;
	    int xmitcsum;
	    int i;
	    int count;
	    char ch;

	    /*
	     * wait around for the start character,
	     * ignore all other characters
	     */
	    while((ch = (char) (getDebugChar() & 0x7f)) != '$') 
	    	;

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
	    	checksum = (char) (checksum + ch);
	    	buffer.append( ch );
	    	count = count + 1;
	    }

	    if (count >= BUFMAX)
    		throw new ChecksumException();

	    if (ch == '#') {
	    	xmitcsum = hex((char) (getDebugChar() & 0x7f)) << 4;
	    	xmitcsum |= hex((char) (getDebugChar() & 0x7f));

	    	if (checksum != xmitcsum)
	    		throw new ChecksumException();

	    }
	    
	    return buffer.toString();
	}
	
	
	
	
	
	
	
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
	
	
	
	
	
	/*
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
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	private String execCmd( String req ) throws IOException, ChecksumException
	{
		int tries = 5;
		while(tries-- > 0)
		{
			try {
				putpacket(req);
				return getpacket();				
			} catch (ChecksumException e) {
				System.err.println("Packet checksum error");
			}
		}
		
		throw new ChecksumException();
	}
	
	
	
    /*
     * mAA..AA,LLLL  Read LLLL bytes at address AA..AA
     */
	public byte[] cmdGetMem(int address, int size) throws CmdException
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
	
}
