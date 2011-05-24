package ru.dz.pdb;

import java.io.File;

import ru.dz.pdb.config.FilePath;

public class Project {
	private File projectFileName = null;
	private String runClassName = null;
	private int runClassMethod = 8;

	private FilePath	sourcePath = new FilePath(
			"../../plib/src" + File.pathSeparator +
			"../../../plib/src" 
			); 

	
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

	public FilePath getSourcePath() {
		return sourcePath;
	}

	public void setSourcePath(FilePath sourcePath) {
		this.sourcePath = sourcePath;
	}

	public File findSourceFile(String name)
	{
		return sourcePath.find(name);
	}
	
}
