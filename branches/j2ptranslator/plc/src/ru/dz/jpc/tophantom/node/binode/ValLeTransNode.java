package ru.dz.jpc.tophantom.node.binode;

import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.binode.ValLeNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * Date: 09.06.2010 16:35:32
 *
 * @author Irek
 */
public class ValLeTransNode extends ValLeNode {
    public ValLeTransNode(Node l, Node r) {
        super(l, r);
    }

    public void find_out_my_type() throws PlcException
    {
      if( type != null && !(type.is_unknown())) return;
      PhantomType l_type = null, r_type = null;

      if( _l != null ) l_type = _l.getType();
      if( _r != null ) r_type = _r.getType();

      if( l_type != null && l_type.equals( r_type ) ) type = l_type;
      else if (".internal.object".equals(l_type.get_class().getName()) && r_type!= null) type = r_type;
      else if (".internal.object".equals(r_type.get_class().getName()) && l_type!= null) type = l_type;
      else type = new PhTypeUnknown();
    }
}
