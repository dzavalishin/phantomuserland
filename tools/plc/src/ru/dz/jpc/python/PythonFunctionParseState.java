package ru.dz.jpc.python;

public class PythonFunctionParseState 
{
	private int funcOutReg = -1;
	private int funcCodeLen = -1;
	private String funcName = null;

	public int getFuncOutReg() {		return funcOutReg;	}
	public void setFuncOutReg(int funcOutReg) {		this.funcOutReg = funcOutReg;	}
	
	public int getFuncCodeLen() {		return funcCodeLen;	}
	public void setFuncCodeLen(int funcCodeLen) {		this.funcCodeLen = funcCodeLen;	}
	
	public String getFuncName() {		return funcName;	}	
	public void setFuncName(String funcName) {		this.funcName = funcName;	}

}
