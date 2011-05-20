package ru.dz.pdb;

import java.io.File;

public class Project {
	private File projectFileName = null;
	private String runClassName = null;
	private int runClassMethod = 8;

	

	
	// --------------------------------------------------------------------
	// Get/set
	// --------------------------------------------------------------------

	public File getProjectFileName() {
		return projectFileName;
	}

	public void setProjectFileName(File projectFileName) {
		this.projectFileName = projectFileName;
	}

	public String getRunClassName() {
		return runClassName ;
	}

	public int getRunClassMethod() {
		return runClassMethod;
	}

	public void setRunClassMethod(int runClassMethod) {
		this.runClassMethod = runClassMethod;
	}

	public void setRunClassName(String runClassName) {
		this.runClassName = runClassName;
	}

	
}
