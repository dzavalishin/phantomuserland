package ru.dz.soot;

import soot.Value;

public class SootExpressionTranslator {

	private Value root;

	public SootExpressionTranslator(Value v) {
		this.root = v;
	}

	public PhantomCodeWrapper process()
	{
		PhantomCodeWrapper ret = doValue(root);
		return ret;
	}

	private PhantomCodeWrapper doValue(Value v) {

		say(" ?? "+v.getClass().getName());
		say("    "+v.toString());

		return PhantomCodeWrapper.getNullNode();
	}
	
	
	private void say(String string) {
		System.err.println(string);
	}
	
}
