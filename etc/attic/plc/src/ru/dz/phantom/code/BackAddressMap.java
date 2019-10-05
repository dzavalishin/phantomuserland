package ru.dz.phantom.code;

import java.util.*;

/**
 * <p>Keeps positions in file (collection!) where an address is to be written.
 * Used for jumps forward (in fact - for any jumps).</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class BackAddressMap
{
  Map<String, LinkedList<Long> > m = Collections.synchronizedMap(new HashMap<String, LinkedList<Long> >());

  public void add( String name, long address )
  {
    //m.put(name, new Integer(address));
    LinkedList<Long> ll = m.get(name);
    if( ll == null )
    {
      ll = new LinkedList<Long>();
      m.put(name,ll);
    }
    ll.addLast(new Long(address));
  }

  /*  public int get( String name ) throws EmptyPlcException {
    Integer io = (Integer)m.get(name);
    if( io == null ) throw new EmptyPlcException("BackAddressMap::get");
    return io.intValue();
  } */

  public LinkedList<Long> get( String name ) { return m.get(name); }

  public void remove( String name ) {
    m.remove(name);
  }


  public Iterator<String> iterator() {
    return m.keySet().iterator();
  }


}

