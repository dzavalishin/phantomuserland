package ru.dz.jpc.tophantom.node;

import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.PhantomType;

/**
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * Date: 29.10.2009 23:57:12
 *
 * @author Irek
 */
public class IdentTransNode extends IdentNode {
    public IdentTransNode(String ident) {
        super(ident);
    }

    public void setType(PhantomType ptype) {
        type = ptype;
    }
}
