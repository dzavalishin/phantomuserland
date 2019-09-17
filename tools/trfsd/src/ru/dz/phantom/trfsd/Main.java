package ru.dz.phantom.trfsd;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;

public class Main {
	public static final int PORT = 2052;

	public static final int SSIZE = 512;

	
	public static final int PKT_T_NOP = 0;
	public static final int PKT_T_Error = 1;
	
	public static final int PKT_T_FindRQ = 11;
	public static final int PKT_T_FindReply = 12;
	public static final int PKT_T_ReadRQ = 13;
	public static final int PKT_T_ReadReply = 14;
	public static final int PKT_T_WriteRQ = 15;
	public static final int PKT_T_WrireReply = 16;

	//private static final int BUF_SIZE = 1024*1024*;

	private static long serverSessionId;
	
	private static DatagramSocket ss;
	
	//private static FileInputStream stream;
	private static RandomAccessFile file;
	
	//private static FileChannel fchannel; 
	
	public static void main(String[] args) throws IOException {
		serverSessionId = System.currentTimeMillis();
		
		String fn = "Phantom.PartitionedDisk";
		/*
		if( false )
		{
			String home = System.getenv("PHANTOM_HOME");
			if( home != null )
			{
				fn = home + "/build/classes";
			}
		}
		*/
		System.out.println("TRFS Daemon, serving from "+fn);
		//new BufferedInputStream( new FileInputStream("Phantom.PageFile"), BUF_SIZE);
		//stream = new FileInputStream("Phantom.ClassFile");
		file = new RandomAccessFile(fn, "r");
		
		if(file == null)
		{
			System.out.println("Unable to open file "+fn);
			return;
		}
		
		ss = new DatagramSocket(PORT);

		int length = 1600;
		
		while(true)
		{
			byte[] buf = new byte[length];
			DatagramPacket p = new DatagramPacket(buf , length);
			ss.receive(p);
			
			try { process( buf, p.getAddress(), p.getPort() ); }
			catch (IOException e) {
				// 5 is EIO errno
				replyWithError(p.getAddress(), p.getPort(), 5, e.getMessage());
				System.out.println("Replied with error "+e.getMessage());
			}
		}
	}


	private static void process(byte[] buf, InetAddress peer, int port) throws IOException {
		ByteBuffer bb = ByteBuffer.wrap(buf);
		bb.order(ByteOrder.LITTLE_ENDIAN);
		
		int pktType = bb.getInt();
		long sessionId = bb.getLong();
		
		System.out.println("Got request type "+pktType +" from "+ peer + " port "+port);
		
		if((sessionId != serverSessionId) && (pktType != PKT_T_FindRQ))
		{
			// 4 is EINTR errno
			replyWithError(peer, port, 4, "Wrong session number");
			System.out.println("Session wrong: "+sessionId+", need "+serverSessionId);			
			return;
		}
		

		switch(pktType) {
		case PKT_T_ReadRQ:
			
			int nRequests = bb.getInt();
			
			while(nRequests-- > 0)
			{
				
				int fileId = bb.getInt();
				int ioId = bb.getInt();
				int nSectors = bb.getInt();
				int startSector = bb.getInt();

				System.out.println("Got ReadRequest for file "+fileId+" sect "+startSector+ " nsect "+nSectors);

				process_read(peer, port, fileId, ioId, nSectors, startSector );
			}
			
			
			break;
		}
		
	}

	
	
	
	
	
	static final int MAX_RQ_SECTORS = 2;

	private static void process_read(InetAddress peer, int port, int fileId, int ioId, int nSectors,
			int startSector ) throws IOException {

		if(fileId != 0)
		{
			// 2 is ENOENT
			replyWithError(peer, port, 2, "Wrong FileId");
			return;
		}

		if(nSectors > MAX_RQ_SECTORS)
		{
			// recurse		
			process_read(peer, port, fileId, ioId, nSectors-MAX_RQ_SECTORS, startSector+MAX_RQ_SECTORS );
			
			nSectors = MAX_RQ_SECTORS;
		}

		
		byte[] data = getData(fileId, nSectors, startSector);
		if(data == null)
		{
			// 14 is EFAULT
			replyWithError(peer, port, 14, "No data");
			return;
		}
		
		
		byte[] buf = new byte[1600];
		
		ByteBuffer bb = ByteBuffer.wrap(buf);
		bb.order(ByteOrder.LITTLE_ENDIAN);
		
		bb.putInt(PKT_T_ReadReply);
		bb.putLong(serverSessionId);
		
		bb.putInt(fileId);
		bb.putInt(ioId);
		bb.putInt(nSectors);
		bb.putLong(startSector);
		
		//bb.putInt(buf.length);
		
		assert(data.length == nSectors*SSIZE);
		
		bb.put(data);
		System.out.println("Sending "+nSectors+" sect. from "+startSector);
		DatagramPacket p = new DatagramPacket(buf, bb.position());
		p.setAddress(peer);
		p.setPort(port);
		ss.send(p);		
		
	}




	private static void replyWithError(InetAddress peer, int port, int errno, String string) throws IOException {
		
		byte[] buf = new byte[1600];
		
		ByteBuffer bb = ByteBuffer.wrap(buf);
		bb.order(ByteOrder.LITTLE_ENDIAN);
		
		bb.putInt(PKT_T_Error);
		bb.putLong(serverSessionId);
		
		bb.putInt(errno);
		
		byte[] strBytes = string.getBytes(Charset.forName("UTF-8"));
		
		bb.putInt(strBytes.length);
		bb.put(strBytes);
		
		DatagramPacket p = new DatagramPacket(buf, bb.position());
		p.setAddress(peer);
		p.setPort(port);
		ss.send(p);		
	}

	
	
	private static byte[] getData(int fileId, int nSectors, int startSector) throws IOException {
		int ioSize = nSectors*SSIZE;
		byte[] data = new byte[ioSize];
		
		file.seek(startSector*SSIZE);
		if( ioSize != file.read(data, 0, ioSize))
			return null;
		
		return data;
	}
	
}
