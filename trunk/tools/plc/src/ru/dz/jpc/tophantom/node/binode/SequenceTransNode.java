package ru.dz.jpc.tophantom.node.binode;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.binode.BiNode;
import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

import java.io.PrintStream;
import java.io.IOException;

/**
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * Date: 02.11.2009 2:14:08
 *
 * @author Irek
 */
public class SequenceTransNode extends SequenceNode {
    public SequenceTransNode(Node l, Node r) {
        super(l, r);
    }


    public boolean is_on_int_stack() {
        return args_on_int_stack();
    }

    public boolean args_on_int_stack() {
        return !go_to_object_stack();
    }

    public boolean go_to_object_stack() {
        boolean result;
        if (_l != null) {
            // first generete code of left node. right node must not take part in calculation.
            result =  (!_l.is_on_int_stack()) /*|| (!_r.is_on_int_stack())*/;
        }
        else if (_r != null) {
            result =  (!_r.is_on_int_stack());
        }
        else {
            result = true;
        }
        return result;
    }


  public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
  {
    if( _l != null ) { _l.generate_code(c,s); move_between_stacks(c, _l.is_on_int_stack()); }
    // after generate right node do not move stacs for right child in SequenceTransNode
    if( _r != null ) { _r.generate_code(c,s); /*move_between_stacks(c, _r.is_on_int_stack());*/ }

    log.fine("Node "+this+" codegen");

    if(context != null) c.emitComment("Line "+context.getLineNumber());
    generate_my_code(c,s);
  }

//    public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
//        _l.generate_code(c, s);
//        if (go_to_object_stack() && _l.is_on_int_stack()) c.emit_i2o();
//        _r.generate_code(c, s);
//        if (go_to_object_stack() && _r.is_on_int_stack()) c.emit_i2o();
//
//        log.fine("Node " + this + " codegen");
//        generate_my_code(c, s);
//    }
}