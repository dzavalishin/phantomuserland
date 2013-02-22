package ru.dz.soot;

import java.util.HashMap;
import java.util.Map;

import soot.Unit;
import soot.UnitBox;

/**
 * Label map - makes labels, mapped to Soot UnitBox'es
 * @author dz
 *
 */


public class SootLabelMap {
	static int nextl = 0;
	
	static private String getNewLabel()
	{
		return String.format("LL%d", nextl++);
	}
	
	private Map<UnitBox,String> labelsUb = new HashMap<UnitBox,String>();
	private Map<Unit,String> labelsU = new HashMap<Unit,String>();

	
	public String getLabelFor(UnitBox o)
	{
		String s = labelsUb.get(o);

		if( s == null )
			s = labelsU.get(o.getUnit());
		
		if( s == null )
		{
			s = getNewLabel();
			labelsUb.put(o, s);
			labelsU.put(o.getUnit(), s);
		}
		
		return s;
	}
	
}
