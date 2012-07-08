/**
 * 
 */
package ru.dz.phantom.ext_tests;

import static org.junit.Assert.*;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.UnknownHostException;
import java.util.Random;

import org.junit.Test;

/**
 * @author dz
 *
 */
public class UdpEchoTest {

	private static final int REDIR_ECHO_PORT = 8007;

	/**
	 * Test method for TCP echo protocol
	 * @throws IOException 
	 * @throws UnknownHostException - no localhost?
	 */
	@Test
	public void testUdpEcho() throws UnknownHostException, IOException {

		DatagramSocket s = new DatagramSocket();
		System.err.format("udp_echo must run in Phantom\n");

		Random rng = new Random();

		long start = System.currentTimeMillis();
		
		final int cnt = 255;
		int nb = cnt;
		while( nb-- > 0 )
		{
			//System.err.format("\r%3d to go", nb);

			byte rnd = (byte) rng.nextInt(256);

			{
				byte[] buf = new byte[2];			
				buf[0] = (byte)nb;
				buf[1] = rnd;

				DatagramPacket p = new DatagramPacket(buf, 2);
				
				p.setAddress(InetAddress.getLocalHost());
				p.setPort(REDIR_ECHO_PORT);
				
				s.send(p);
				
				//System.err.format(" w");
			}

			{
				byte[] buf = new byte[2];			

				DatagramPacket p = new DatagramPacket(buf, 2);
				s.receive(p);
				//System.err.format(" r port %d", p.getPort());

				assertEquals(nb, 0xFF & buf[0]);
				assertEquals(0xFF & rnd, 0xFF & buf[1]);
			}

		}

		long end = System.currentTimeMillis();

		long diff = (end-start)/cnt;
		
		System.err.format("\r\ndone, %d msec/pkt", diff );

		
		s.close();
	}

}
