package ru.dz.soot;

import java.util.HashMap;
import java.util.Map;

import soot.UnitBox;

/**
 * Label map - makes labels;
 * @author dz
 *
 */


public class SootLabelMap {
	static int nextl = 0;
	
	static String getNewLabel()
	{
		return String.format("SootL%d", nextl++);
	}
	
	private Map<UnitBox,String> labels = new HashMap<UnitBox,String>();

	
	public String getLabelFor(UnitBox o)
	{
		String s = labels.get(o);
		if( s == null )
		{
			s = getNewLabel();
			labels.put(o, s);
		}
		return s;
	}
	
}
