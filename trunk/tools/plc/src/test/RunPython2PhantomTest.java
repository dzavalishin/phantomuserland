package test;

import static org.junit.Assert.*;

import java.io.IOException;

import javax.xml.parsers.ParserConfigurationException;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.xml.sax.SAXException;

import ru.dz.jpc.python.ConnvertException;
import ru.dz.jpc.python.PpcMain;
import ru.dz.plc.util.PlcException;

public class RunPython2PhantomTest {

	@Before
	public void setUp() throws Exception {
	}

	@After
	public void tearDown() throws Exception {
	}

	@Test
	public void test() throws ParserConfigurationException, SAXException, IOException, ConnvertException, PlcException {
		String[] args = new String[0];
		
		if(PpcMain.go(args))		
			fail("Nonzero exit");
	}

}
