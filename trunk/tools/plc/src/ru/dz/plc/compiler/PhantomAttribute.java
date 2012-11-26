package ru.dz.plc.compiler;

import java.util.*;

import ru.dz.plc.util.*;

/**
 * <p>Attribute.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public abstract class PhantomAttribute
{
  public PhantomAttribute() {  }

  public boolean is_want() { return false; }
  public boolean is_not() { return false; }
  public abstract String get_name();

  public boolean compatible_with(PhantomAttribute him)
  {
    if( !get_name().equals(him.get_name()) ) return false;

    if( is_not() && him != null && !him.is_not()) return false;
    if( is_want() && him == null ) return false;
    if( is_want() && him != null && him.is_not()) return false;

    if( him != null && him.is_want() && is_not() ) return false;

    return true;
  }

  public PhantomAttribute combine_with(PhantomAttribute him) throws PlcException {

    if( !get_name().equals(him.get_name()) )
      throw new PlcException("PhantomAttribute combine_with","different names",get_name()+" and "+him.get_name());

    if( !compatible_with( him ) )
      throw new PlcException("PhantomAttribute combine_with","incompatible attributes",toString()+" and "+him.toString());

    // Same attrs? Just the same result.
    if( him != null && equals(him) )
      return this;

    boolean is_additive = false;
    // For not attrs additive and multiplicative work vice versa
    if( is_not() ) is_additive = !is_additive;

    // check if he is null and I am additive, then return this
    if( him == null && is_additive )
      return this;

    // well, i am not additive
    return null;
  }

  public String toString() {
    return
        "attribute "+
        (is_not() ? "~" : "")+
        get_name()+
        (is_want() ? "~" : "")
        ;
  }

}

class ph_may_attribute extends PhantomAttribute
{
  private String name;

  public ph_may_attribute( String name ) { this.name = name; }

  public String get_name() { return name; }

}

class ph_want_attribute extends ph_may_attribute
{
  public ph_want_attribute( String name ) { super(name); }

  public boolean is_want() { return true; }
}


class ph_not_attribute extends PhantomAttribute
{
  PhantomAttribute base;
  public boolean is_not() { return true; }
  public String get_name() { return base.get_name(); }

  ph_not_attribute( PhantomAttribute _base ) throws PlcException {
    if( _base.is_not() ) throw new PlcException( "ph_not_attribute c'tor", "can't be based on ph_not_attribute", base.toString() );
    this.base = (PhantomAttribute)_base;
  }

  ph_not_attribute( String name, Map<String, PhantomAttribute> attribute_map ) throws PlcException {
    base = attribute_map.get(name);
    if( base.is_not() ) throw new PlcException( "ph_not_attribute c'tor", "can't be based on ph_not_attribute", base.toString() );
  }

}
