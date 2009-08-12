package ru.dz.phantom.snapdump;

import java.util.HashMap;
import java.util.Map;

public class VisitedMap {
	private Map<Integer,VisitedState> map = new HashMap<Integer,VisitedState>();
	
	public VisitedState visit(int address)
	{
		VisitedState state = map.get(address);
		
		if(state == null)
			state = new VisitedState();
		
		map.put(address, state);
		return state;
	}

	public VisitedState get(int address)
	{
		return map.get(address);
	}

	public boolean isVisited(int address)
	{
		return map.get(address) != null;
	}
	
}
