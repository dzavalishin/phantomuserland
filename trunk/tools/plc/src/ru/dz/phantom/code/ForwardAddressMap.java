package ru.dz.phantom.code;

import java.util.*;

import ru.dz.plc.util.*;

/**
 * <p>Keeps label address to name map.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class ForwardAddressMap
{
  //public 
  private Map<String, Long> m = Collections.synchronizedMap(new HashMap<String, Long>());

  public void add( String name, long address )
  {
    m.put(name, new Long(address));
  }

  public long get( String name ) throws EmptyPlcException {
    Long io = (Long)m.get(name);
    if( io == null ) throw new EmptyPlcException("BackAddressMap::get");
    return io.longValue();
  }
  
  public Iterator<String> iterator() { return m.keySet().iterator(); }
}

