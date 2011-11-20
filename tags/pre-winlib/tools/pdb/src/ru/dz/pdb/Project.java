package ru.dz.pdb;

import java.io.File;

import javax.xml.bind.annotation.XmlTransient;
import javax.xml.bind.annotation.adapters.XmlAdapter;
import javax.xml.bind.annotation.adapters.XmlJavaTypeAdapter;

import ru.dz.pdb.config.FilePath;

public class Project {
	private String projectName = "NewProject"; 
	
	
	@XmlJavaTypeAdapter(FileXMLAdapter.class)
	private File projectFileName = null;

	private String runClassName = null;
	private int runClassMethod = 8;

	private FilePath	sourcePath = new FilePath(
			"../../plib/src" + File.pathSeparator +
			"../../../plib/src" 
			);
	
	private FilePath binaryPath = new FilePath(
			"../../plib/bin" + File.pathSeparator +
			"../../../plib/bin" 
			);
	

	
	// --------------------------------------------------------------------
	// Get/set
	// --------------------------------------------------------------------

	@XmlJavaTypeAdapter(FileXMLAdapter.class)
	public File getProjectFileName() {		return projectFileName;	}
	@XmlJavaTypeAdapter(FileXMLAdapter.class)
	public void setProjectFileName(File projectFileName) {		this.projectFileName = projectFileName;	}

	public String getRunClassName() {		return runClassName ;	}
	public int getRunClassMethod() {		return runClassMethod;	}

	public void setRunClassMethod(int runClassMethod) {		this.runClassMethod = runClassMethod;	}
	public void setRunClassName(String runClassName) {		this.runClassName = runClassName;	}

	public FilePath getSourcePath() {		return sourcePath;	}
	public void setSourcePath(FilePath sourcePath) {		this.sourcePath = sourcePath;	}

	public File findSourceFile(String name)
	{
		return sourcePath.find(name);
	}

	public FilePath getBinaryPath() {		return binaryPath;	}
	public void setBinaryPath(FilePath path) {		this.binaryPath = path;	}

	public File findBinaryFile(String name)
	{
		return binaryPath.find(name);
	}
	
	public String getProjectName() {		return projectName;	}
	public void setProjectName(String projectName) {		this.projectName = projectName;	}
	
}

final class FileXMLAdapter extends XmlAdapter<String,File>
{

	@Override
	public String marshal(File v) throws Exception {
		return v.toString();
	}

	@Override
	public File unmarshal(String v) throws Exception {
		return new File(v);
	}
	
}
