package ru.dz.plc.compiler.node;

public class SwitchDefaultNode extends SwitchLabelNode {
	public SwitchDefaultNode(String label) {        super(label,null);    }
	public String toString()  {    return "default "; }
}
