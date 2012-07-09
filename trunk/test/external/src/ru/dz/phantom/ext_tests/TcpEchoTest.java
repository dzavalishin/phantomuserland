/**
 * 
 */
package ru.dz.phantom.ext_tests;

import static org.junit.Assert.*;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Random;

import org.junit.Test;

/**
 * @author dz
 *
 */
public class TcpEchoTest {

	private static final int REDIR_ECHO_PORT = 8007;

	public TcpEchoTest() 
	{
		System.err.format("tcp_echo must run in Phantom\n");

	}
	
	/**
	 * Test method for TCP echo protocol
	 * @throws IOException 
	 * @throws UnknownHostException - no localhost?
	 */
	@Test
	public void testTcpEcho() throws UnknownHostException, IOException {
		
		//if(true) return;
		
		Socket s = new Socket(InetAddress.getLocalHost(), REDIR_ECHO_PORT);

		OutputStream os = s.getOutputStream();
		InputStream is = s.getInputStream();
		
		Random rng = new Random();
		
		int nb  = 200;
		while( nb-- > 0 )
		{
			System.err.format("\r%3d to go", nb);
			
			byte rnd = (byte) rng.nextInt(256);
			
			os.write((byte)nb);
			System.err.format(" w1");
			os.write(rnd);
			System.err.format(" w2");
			
			int r1 = is.read();
			int r2 = is.read();
			
			assertEquals(nb, r1);
			System.err.format(" r1");
			assertEquals(rnd, r2);
			System.err.format(" r2");
		}
		
		s.close();
		
		//fail("Not yet implemented");
	}

}
