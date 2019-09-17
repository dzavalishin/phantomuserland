
package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Div reminder node.</p>
 * <p>Copyright: Copyright (c) 2004-2011 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class OpRemainderNode extends BiNode
{
    public OpRemainderNode(Node l, Node r) {    super(l,r);  }
    public String toString()  {    return "%";  }
    public boolean is_on_int_stack() { return true; }
    protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
    {
        if(getType().is_int()) c.emitIRemLU();
        else throw new PlcException("Codegen", "op % does not exist for this type");
    }
}
