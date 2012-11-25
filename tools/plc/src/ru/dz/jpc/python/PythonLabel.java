package ru.dz.jpc.python;

import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;

public class PythonLabel {
	private String name;

	public PythonLabel( Node domNode) {
		NamedNodeMap attributes = domNode.getAttributes();
		Node namedItem = attributes.getNamedItem("name");
		name = (namedItem == null) ? null : namedItem.getNodeValue();
	}

	public String getName() {		return name;	}

	@Override
	public String toString() {
		return "label "+name;
	}
}
