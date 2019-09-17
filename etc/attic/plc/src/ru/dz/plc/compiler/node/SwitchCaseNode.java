package ru.dz.plc.compiler.node;

public class SwitchCaseNode extends SwitchLabelNode {
	public SwitchCaseNode( String label, Node expr ) {  super(label,expr);  }
	public String toString()  {    return "case "; }
}