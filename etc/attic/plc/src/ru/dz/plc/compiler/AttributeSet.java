package ru.dz.plc.compiler;
import java.util.*;

import ru.dz.plc.util.*;

/**
 * <p>Attributes.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class AttributeSet {
	private HashMap<String,PhantomAttribute> map;

  public AttributeSet() {
  }

  public void add( PhantomAttribute a )
  {
    map.put(a.get_name(),a);
  }

  public PhantomAttribute get( String name )
  {
    return map.get(name);
  }

  boolean can_receive_object_with_attributes( AttributeSet src_orig )
  {
    HashMap<String,PhantomAttribute> src = src_orig.map;
    HashMap<String,PhantomAttribute> dst = map;

    Iterator<PhantomAttribute> i = dst.values().iterator();
    while( i.hasNext() )
    {
      PhantomAttribute dst_a = (PhantomAttribute)i.next();
      PhantomAttribute src_a = (PhantomAttribute)src.get(dst_a.get_name());

      if( !dst_a.compatible_with(src_a) ) return false;
    }

    i = src.values().iterator();
    while( i.hasNext() )
    {
      PhantomAttribute src_a = (PhantomAttribute)i.next();
      PhantomAttribute dst_a = (PhantomAttribute)get(src_a.get_name());

      if( !src_a.compatible_with(dst_a) ) return false;
    }

    return true;
  }

  static AttributeSet combine( AttributeSet l, AttributeSet r ) throws PlcException {
    AttributeSet out = new AttributeSet();

    HashMap<String,PhantomAttribute> lam = l.map;
    HashMap<String,PhantomAttribute> ram = r.map;

    Iterator<PhantomAttribute> i = lam.values().iterator();
    while( i.hasNext() )
    {
      PhantomAttribute la = (PhantomAttribute)i.next();
      PhantomAttribute ra = (PhantomAttribute)ram.get(la.get_name());

      if( !la.compatible_with(ra) )
        throw new PlcException("AttributeSet combine", "incompatible attributes", la.toString() );

      if( ra != null )
      {
        PhantomAttribute newa = la.combine_with(ra);
        if( newa != null ) out.add(newa);
      }
    }

    i = ram.values().iterator();
    while( i.hasNext() )
    {
      PhantomAttribute ra = (PhantomAttribute)i.next();
      PhantomAttribute la = (PhantomAttribute)lam.get(ra.get_name());

      if( !ra.compatible_with(la) )
        throw new PlcException("AttributeSet combine", "incompatible attributes", la.toString() );

      if( la != null && !out.map.containsKey(ra.get_name()))
      {
        PhantomAttribute newa = ra.combine_with(la);
        if( newa != null ) out.add(newa);
      }
    }

    return out;
  }

  public String toString()
  {
    StringBuffer sb = new StringBuffer();
    boolean first = true;

    Iterator<PhantomAttribute> i = map.values().iterator();
    while( i.hasNext() )
    {
      PhantomAttribute a = (PhantomAttribute)i.next();
      if(!first) sb.append(", ");
      sb.append(a.toString());
      first = false;
    }

    return sb.toString();
  }


}




